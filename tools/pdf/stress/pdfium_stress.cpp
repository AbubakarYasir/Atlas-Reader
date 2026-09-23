#define NOMINMAX
#include <windows.h>
#include <psapi.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <fpdf_text.h>
#include <fpdfview.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct MemorySample {
    int iteration = 0;
    quint64 workingSetBytes = 0;
};

struct LifetimeResult {
    int requested = 0;
    int completed = 0;
    int renderSuccesses = 0;
    int textSuccesses = 0;
    quint64 startWorkingSet = 0;
    quint64 endWorkingSet = 0;
    quint64 observedPeak = 0;
    double elapsedMs = 0.0;
    std::vector<MemorySample> checkpoints;
    std::vector<QString> failures;
};

struct QueueTask {
    int id = 0;
    const QByteArray* bytes = nullptr;
    Clock::time_point enqueuedAt;
};

struct QueueResult {
    int producerCount = 0;
    int requestsPerProducer = 0;
    int submitted = 0;
    int completed = 0;
    int failed = 0;
    int maxActiveApiExecutions = 0;
    int pdfiumWorkerThreadCount = 0;
    int producerPdfiumApiCalls = 0;
    bool cleanShutdown = false;
    std::vector<double> queueWaitMs;
    std::vector<double> totalLatencyMs;
    std::vector<QString> failures;
};

quint64 currentWorkingSetBytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters))) {
        return 0;
    }
    return static_cast<quint64>(counters.WorkingSetSize);
}

quint64 peakWorkingSetBytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters))) {
        return 0;
    }
    return static_cast<quint64>(counters.PeakWorkingSetSize);
}

std::optional<int> parsePositive(const char* value)
{
    bool ok = false;
    const int parsed = QString::fromLocal8Bit(value).toInt(&ok);
    if (!ok || parsed <= 0) {
        return std::nullopt;
    }
    return parsed;
}

bool readFile(const QString& path, QByteArray* output)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    *output = file.readAll();
    return !output->isEmpty();
}

bool runPdfiumWork(const QByteArray& bytes, int renderWidth, int renderHeight, QString* failure)
{
    FPDF_DOCUMENT document = FPDF_LoadMemDocument64(
        bytes.constData(), static_cast<std::size_t>(bytes.size()), nullptr);
    if (document == nullptr || FPDF_GetPageCount(document) < 1) {
        *failure = QStringLiteral("open");
        if (document != nullptr) {
            FPDF_CloseDocument(document);
        }
        return false;
    }

    FPDF_PAGE page = FPDF_LoadPage(document, 0);
    if (page == nullptr) {
        *failure = QStringLiteral("page-load");
        FPDF_CloseDocument(document);
        return false;
    }

    FPDF_BITMAP bitmap = FPDFBitmap_Create(renderWidth, renderHeight, 1);
    if (bitmap == nullptr) {
        *failure = QStringLiteral("bitmap-create");
        FPDF_ClosePage(page);
        FPDF_CloseDocument(document);
        return false;
    }

    FPDFBitmap_FillRect(bitmap, 0, 0, renderWidth, renderHeight, 0xFFFFFFFFU);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, renderWidth, renderHeight, 0, 0);

    bool ok = FPDFBitmap_GetBuffer(bitmap) != nullptr;
    if (!ok) {
        *failure = QStringLiteral("render");
    }

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    if (textPage == nullptr) {
        ok = false;
        *failure = QStringLiteral("text-page-load");
    } else {
        const int charCount = FPDFText_CountChars(textPage);
        if (charCount <= 0) {
            ok = false;
            *failure = QStringLiteral("text-empty");
        }
        FPDFText_ClosePage(textPage);
    }

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);
    FPDF_CloseDocument(document);
    return ok;
}

LifetimeResult runLifetime(
    const QByteArray& a003,
    const QByteArray& a011,
    int iterations,
    int renderWidth,
    int renderHeight)
{
    constexpr int checkpointEvery = 50;
    LifetimeResult result;
    result.requested = iterations;
    result.startWorkingSet = currentWorkingSetBytes();
    result.observedPeak = result.startWorkingSet;
    const auto started = Clock::now();

    for (int iteration = 1; iteration <= iterations; ++iteration) {
        const QByteArray& bytes = (iteration % 2 == 0) ? a011 : a003;
        QString failure;
        if (runPdfiumWork(bytes, renderWidth, renderHeight, &failure)) {
            ++result.completed;
            ++result.renderSuccesses;
            ++result.textSuccesses;
        } else {
            result.failures.push_back(QStringLiteral("iteration-%1:%2").arg(iteration).arg(failure));
        }

        if (iteration % checkpointEvery == 0 || iteration == iterations) {
            const quint64 current = currentWorkingSetBytes();
            result.observedPeak = (std::max)(result.observedPeak, current);
            result.checkpoints.push_back(MemorySample{iteration, current});
        }
    }

    result.endWorkingSet = currentWorkingSetBytes();
    result.observedPeak = (std::max)(result.observedPeak, result.endWorkingSet);
    result.elapsedMs = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
    return result;
}

double percentileNearestRank(std::vector<double> values, double percentile)
{
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double rank = std::ceil(percentile * static_cast<double>(values.size()));
    const std::size_t index = static_cast<std::size_t>((std::max)(1.0, rank) - 1.0);
    return values[(std::min)(index, values.size() - 1U)];
}

QJsonObject memorySampleJson(const MemorySample& sample)
{
    QJsonObject out;
    out.insert(QStringLiteral("iteration"), sample.iteration);
    out.insert(QStringLiteral("working_set_bytes"), static_cast<qint64>(sample.workingSetBytes));
    out.insert(QStringLiteral("working_set_mib"), static_cast<double>(sample.workingSetBytes) / 1048576.0);
    return out;
}

QJsonArray stringArray(const std::vector<QString>& values)
{
    QJsonArray out;
    for (const QString& value : values) {
        out.append(value);
    }
    return out;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 7) {
        std::fprintf(
            stderr,
            "usage: atlas_pdfium_stress <A003.pdf> <A011.pdf> <lifetime-iterations> <producer-count> <requests-per-producer> <render-size>\n");
        return 64;
    }

    const QString a003Path = QString::fromLocal8Bit(argv[1]);
    const QString a011Path = QString::fromLocal8Bit(argv[2]);
    const auto lifetimeIterations = parsePositive(argv[3]);
    const auto producerCount = parsePositive(argv[4]);
    const auto requestsPerProducer = parsePositive(argv[5]);
    const auto renderSize = parsePositive(argv[6]);
    if (!lifetimeIterations || !producerCount || !requestsPerProducer || !renderSize) {
        std::fprintf(stderr, "all numeric arguments must be positive integers\n");
        return 64;
    }

    QByteArray a003Bytes;
    QByteArray a011Bytes;
    if (!readFile(a003Path, &a003Bytes) || !readFile(a011Path, &a011Bytes)) {
        std::fprintf(stderr, "failed to read stress fixtures\n");
        return 66;
    }

    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::queue<QueueTask> queue;
    bool producersDone = false;

    std::mutex lifetimeMutex;
    std::condition_variable lifetimeCv;
    bool lifetimeDone = false;

    LifetimeResult lifetime;
    QueueResult queueResult;
    queueResult.producerCount = *producerCount;
    queueResult.requestsPerProducer = *requestsPerProducer;

    std::atomic<int> activeApi{0};
    std::atomic<int> maxActiveApi{0};
    std::atomic<int> submitted{0};

    const quint64 processStartWorkingSet = currentWorkingSetBytes();

    std::thread worker([&]() {
        FPDF_InitLibrary();

        lifetime = runLifetime(a003Bytes, a011Bytes, *lifetimeIterations, 306, 396);
        {
            std::lock_guard<std::mutex> lock(lifetimeMutex);
            lifetimeDone = true;
        }
        lifetimeCv.notify_one();

        queueResult.pdfiumWorkerThreadCount = 1;
        for (;;) {
            QueueTask task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCv.wait(lock, [&]() { return producersDone || !queue.empty(); });
                if (queue.empty() && producersDone) {
                    break;
                }
                task = queue.front();
                queue.pop();
            }

            const auto start = Clock::now();
            queueResult.queueWaitMs.push_back(
                std::chrono::duration<double, std::milli>(start - task.enqueuedAt).count());

            const int activeNow = activeApi.fetch_add(1) + 1;
            int observed = maxActiveApi.load();
            while (activeNow > observed && !maxActiveApi.compare_exchange_weak(observed, activeNow)) {
            }

            QString failure;
            const bool ok = runPdfiumWork(*task.bytes, *renderSize, *renderSize, &failure);
            activeApi.fetch_sub(1);

            const auto finished = Clock::now();
            queueResult.totalLatencyMs.push_back(
                std::chrono::duration<double, std::milli>(finished - task.enqueuedAt).count());

            if (ok) {
                ++queueResult.completed;
            } else {
                ++queueResult.failed;
                queueResult.failures.push_back(QStringLiteral("task-%1:%2").arg(task.id).arg(failure));
            }
        }

        queueResult.maxActiveApiExecutions = maxActiveApi.load();
        queueResult.cleanShutdown = true;
        FPDF_DestroyLibrary();
    });

    {
        std::unique_lock<std::mutex> lock(lifetimeMutex);
        lifetimeCv.wait(lock, [&]() { return lifetimeDone; });
    }

    std::vector<std::thread> producers;
    producers.reserve(static_cast<std::size_t>(*producerCount));
    for (int producer = 0; producer < *producerCount; ++producer) {
        producers.emplace_back([&, producer]() {
            for (int request = 0; request < *requestsPerProducer; ++request) {
                const int id = producer * (*requestsPerProducer) + request;
                QueueTask task;
                task.id = id;
                task.bytes = (id % 2 == 0) ? &a003Bytes : &a011Bytes;
                task.enqueuedAt = Clock::now();
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    queue.push(task);
                }
                submitted.fetch_add(1);
                queueCv.notify_one();
            }
        });
    }

    for (std::thread& producer : producers) {
        producer.join();
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        producersDone = true;
    }
    queueCv.notify_one();
    worker.join();

    queueResult.submitted = submitted.load();
    const quint64 processEndWorkingSet = currentWorkingSetBytes();
    const quint64 osPeak = peakWorkingSetBytes();

    QJsonArray checkpointJson;
    for (const MemorySample& sample : lifetime.checkpoints) {
        checkpointJson.append(memorySampleJson(sample));
    }

    QJsonObject lifetimeJson;
    lifetimeJson.insert(QStringLiteral("iterations_requested"), lifetime.requested);
    lifetimeJson.insert(QStringLiteral("iterations_completed"), lifetime.completed);
    lifetimeJson.insert(QStringLiteral("render_successes"), lifetime.renderSuccesses);
    lifetimeJson.insert(QStringLiteral("text_successes"), lifetime.textSuccesses);
    lifetimeJson.insert(QStringLiteral("working_set_start_bytes"), static_cast<qint64>(lifetime.startWorkingSet));
    lifetimeJson.insert(QStringLiteral("working_set_end_bytes"), static_cast<qint64>(lifetime.endWorkingSet));
    lifetimeJson.insert(
        QStringLiteral("working_set_end_minus_start_bytes"),
        static_cast<qint64>(lifetime.endWorkingSet) - static_cast<qint64>(lifetime.startWorkingSet));
    lifetimeJson.insert(QStringLiteral("observed_checkpoint_peak_bytes"), static_cast<qint64>(lifetime.observedPeak));
    lifetimeJson.insert(QStringLiteral("elapsed_ms"), lifetime.elapsedMs);
    lifetimeJson.insert(QStringLiteral("checkpoints"), checkpointJson);
    lifetimeJson.insert(QStringLiteral("failures"), stringArray(lifetime.failures));
    lifetimeJson.insert(
        QStringLiteral("passed"),
        lifetime.failures.empty() && lifetime.completed == lifetime.requested);

    QJsonObject queueJson;
    queueJson.insert(QStringLiteral("producer_count"), queueResult.producerCount);
    queueJson.insert(QStringLiteral("requests_per_producer"), queueResult.requestsPerProducer);
    queueJson.insert(QStringLiteral("submitted"), queueResult.submitted);
    queueJson.insert(QStringLiteral("completed"), queueResult.completed);
    queueJson.insert(QStringLiteral("failed"), queueResult.failed);
    queueJson.insert(QStringLiteral("pdfium_worker_thread_count"), queueResult.pdfiumWorkerThreadCount);
    queueJson.insert(QStringLiteral("producer_pdfium_api_calls"), queueResult.producerPdfiumApiCalls);
    queueJson.insert(QStringLiteral("max_active_pdfium_api_executions"), queueResult.maxActiveApiExecutions);
    queueJson.insert(QStringLiteral("clean_shutdown"), queueResult.cleanShutdown);
    queueJson.insert(QStringLiteral("queue_wait_p50_ms"), percentileNearestRank(queueResult.queueWaitMs, 0.50));
    queueJson.insert(QStringLiteral("queue_wait_p95_ms"), percentileNearestRank(queueResult.queueWaitMs, 0.95));
    queueJson.insert(QStringLiteral("total_latency_p50_ms"), percentileNearestRank(queueResult.totalLatencyMs, 0.50));
    queueJson.insert(QStringLiteral("total_latency_p95_ms"), percentileNearestRank(queueResult.totalLatencyMs, 0.95));
    queueJson.insert(QStringLiteral("failures"), stringArray(queueResult.failures));

    const int expectedSubmitted = (*producerCount) * (*requestsPerProducer);
    const bool queuePassed = queueResult.cleanShutdown
        && queueResult.submitted == expectedSubmitted
        && queueResult.completed == expectedSubmitted
        && queueResult.failed == 0
        && queueResult.pdfiumWorkerThreadCount == 1
        && queueResult.producerPdfiumApiCalls == 0
        && queueResult.maxActiveApiExecutions == 1;
    queueJson.insert(QStringLiteral("passed"), queuePassed);

    const bool lifetimePassed = lifetimeJson.value(QStringLiteral("passed")).toBool();

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-stress.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("pdfium_pin"), QStringLiteral(ATLAS_PDFIUM_PIN));
    result.insert(QStringLiteral("pdfium_version"), QStringLiteral(ATLAS_PDFIUM_VERSION));
    result.insert(QStringLiteral("call_model"), QStringLiteral("single-dedicated-worker-thread"));
    result.insert(QStringLiteral("library_init_thread"), QStringLiteral("pdfium-worker"));
    result.insert(QStringLiteral("library_shutdown_thread"), QStringLiteral("pdfium-worker"));
    result.insert(QStringLiteral("lifetime"), lifetimeJson);
    result.insert(QStringLiteral("serialized_queue"), queueJson);
    result.insert(QStringLiteral("process_working_set_start_bytes"), static_cast<qint64>(processStartWorkingSet));
    result.insert(QStringLiteral("process_working_set_end_bytes"), static_cast<qint64>(processEndWorkingSet));
    result.insert(QStringLiteral("process_os_peak_working_set_bytes"), static_cast<qint64>(osPeak));
    result.insert(QStringLiteral("passed"), lifetimePassed && queuePassed);

    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return result.value(QStringLiteral("passed")).toBool() ? 0 : 2;
}
