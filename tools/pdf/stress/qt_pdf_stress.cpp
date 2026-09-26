#include <QCoreApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QSize>
#include <QString>

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <optional>

namespace {

struct MemorySample {
    int iteration = 0;
    quint64 workingSetBytes = 0;
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

QJsonObject sampleJson(const MemorySample& sample)
{
    QJsonObject out;
    out.insert(QStringLiteral("iteration"), sample.iteration);
    out.insert(QStringLiteral("working_set_bytes"), static_cast<qint64>(sample.workingSetBytes));
    out.insert(QStringLiteral("working_set_mib"), static_cast<double>(sample.workingSetBytes) / 1048576.0);
    return out;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        std::fprintf(stderr, "usage: atlas_qt_pdf_stress <A003.pdf> <A011.pdf> <iterations>\n");
        return 64;
    }

    const QString a003Path = QString::fromLocal8Bit(argv[1]);
    const QString a011Path = QString::fromLocal8Bit(argv[2]);
    const auto iterations = parsePositive(argv[3]);
    if (!iterations.has_value()) {
        std::fprintf(stderr, "iterations must be a positive integer\n");
        return 64;
    }

    constexpr int renderWidth = 306;
    constexpr int renderHeight = 396;
    constexpr int checkpointEvery = 50;

    QJsonArray failures;
    QJsonArray checkpoints;
    int completed = 0;
    int renderSuccesses = 0;
    int textSuccesses = 0;
    quint64 observedPeak = 0;

    const quint64 startWorkingSet = currentWorkingSetBytes();
    observedPeak = startWorkingSet;

    QElapsedTimer totalTimer;
    totalTimer.start();

    for (int iteration = 1; iteration <= *iterations; ++iteration) {
        const QString& path = (iteration % 2 == 0) ? a011Path : a003Path;
        bool iterationPassed = true;

        {
            QPdfDocument document;
            const QPdfDocument::Error error = document.load(path);
            if (error != QPdfDocument::Error::None || document.pageCount() < 1) {
                QJsonObject failure;
                failure.insert(QStringLiteral("iteration"), iteration);
                failure.insert(QStringLiteral("operation"), QStringLiteral("open"));
                failure.insert(QStringLiteral("error_code"), static_cast<int>(error));
                failures.append(failure);
                iterationPassed = false;
            } else {
                const QImage image = document.render(0, QSize(renderWidth, renderHeight));
                if (image.isNull() || image.width() != renderWidth || image.height() != renderHeight) {
                    QJsonObject failure;
                    failure.insert(QStringLiteral("iteration"), iteration);
                    failure.insert(QStringLiteral("operation"), QStringLiteral("render"));
                    failures.append(failure);
                    iterationPassed = false;
                } else {
                    ++renderSuccesses;
                }

                const QPdfSelection selection = document.getAllText(0);
                if (selection.text().isEmpty()) {
                    QJsonObject failure;
                    failure.insert(QStringLiteral("iteration"), iteration);
                    failure.insert(QStringLiteral("operation"), QStringLiteral("extract"));
                    failures.append(failure);
                    iterationPassed = false;
                } else {
                    ++textSuccesses;
                }
            }
        }

        if (iterationPassed) {
            ++completed;
        }

        if (iteration % checkpointEvery == 0 || iteration == *iterations) {
            const quint64 current = currentWorkingSetBytes();
            observedPeak = (std::max)(observedPeak, current);
            checkpoints.append(sampleJson(MemorySample{iteration, current}));
        }
    }

    const double totalElapsedMs = static_cast<double>(totalTimer.nsecsElapsed()) / 1'000'000.0;
    const quint64 endWorkingSet = currentWorkingSetBytes();
    observedPeak = (std::max)(observedPeak, endWorkingSet);
    const quint64 osPeak = peakWorkingSetBytes();

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-stress.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("iterations_requested"), *iterations);
    result.insert(QStringLiteral("iterations_completed"), completed);
    result.insert(QStringLiteral("render_successes"), renderSuccesses);
    result.insert(QStringLiteral("text_successes"), textSuccesses);
    result.insert(QStringLiteral("render_width"), renderWidth);
    result.insert(QStringLiteral("render_height"), renderHeight);
    result.insert(QStringLiteral("checkpoint_every"), checkpointEvery);
    result.insert(QStringLiteral("working_set_start_bytes"), static_cast<qint64>(startWorkingSet));
    result.insert(QStringLiteral("working_set_end_bytes"), static_cast<qint64>(endWorkingSet));
    result.insert(QStringLiteral("working_set_end_minus_start_bytes"), static_cast<qint64>(endWorkingSet) - static_cast<qint64>(startWorkingSet));
    result.insert(QStringLiteral("observed_checkpoint_peak_bytes"), static_cast<qint64>(observedPeak));
    result.insert(QStringLiteral("os_peak_working_set_bytes"), static_cast<qint64>(osPeak));
    result.insert(QStringLiteral("total_elapsed_ms"), totalElapsedMs);
    result.insert(QStringLiteral("checkpoints"), checkpoints);
    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty() && completed == *iterations);

    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return result.value(QStringLiteral("passed")).toBool() ? 0 : 2;
}
