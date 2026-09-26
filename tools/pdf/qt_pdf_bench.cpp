#include "bench_common.h"

#include <QBuffer>
#include <QByteArray>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QModelIndex>
#include <QPdfDocument>
#include <QPdfSearchModel>
#include <QSize>
#include <QStringList>
#include <QThread>

#include <cstddef>
#include <cstdio>
#include <optional>
#include <vector>

namespace {

struct SearchTiming {
    double elapsedMs = 0.0;
    int hitCount = 0;
    bool completed = false;
};

std::optional<int> parseInt(const QString& value)
{
    bool ok = false;
    const int parsed = value.toInt(&ok);
    return ok ? std::optional<int>(parsed) : std::nullopt;
}

QString sha256(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

void addFailure(QJsonArray& failures, const QString& message)
{
    failures.append(message);
}

bool awaitDocumentReady(QPdfDocument& document, int timeoutMs = 5000)
{
    QElapsedTimer timeout;
    timeout.start();
    while (document.status() == QPdfDocument::Status::Loading && timeout.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    return document.status() == QPdfDocument::Status::Ready
        && document.error() == QPdfDocument::Error::None;
}

double openOnce(const QByteArray& pdfBytes, bool* ok)
{
    QBuffer buffer;
    buffer.setData(pdfBytes);
    if (!buffer.open(QIODevice::ReadOnly)) {
        *ok = false;
        return 0.0;
    }

    QPdfDocument document;
    QElapsedTimer timer;
    timer.start();
    document.load(&buffer);
    const bool ready = awaitDocumentReady(document);
    const double elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    *ok = ready && document.pageCount() > 0;
    return elapsedMs;
}

double extractAllPages(QPdfDocument& document, qsizetype* textLength)
{
    qsizetype totalLength = 0;
    QElapsedTimer timer;
    timer.start();
    for (int page = 0; page < document.pageCount(); ++page) {
        totalLength += document.getAllText(page).text().size();
    }
    const double elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    *textLength = totalLength;
    return elapsedMs;
}

SearchTiming searchOnce(QPdfDocument& document, const QString& query, int expectedHits, int timeoutMs)
{
    QPdfSearchModel model;
    model.setDocument(&document);
    const QModelIndex root;

    QElapsedTimer timer;
    timer.start();
    model.setSearchString(query);

    int hitCount = model.rowCount(root);
    while (timer.elapsed() < timeoutMs && hitCount < expectedHits) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        hitCount = model.rowCount(root);
        if (hitCount >= expectedHits) {
            break;
        }
        QThread::msleep(1);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    hitCount = model.rowCount(root);

    return {
        static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0,
        hitCount,
        hitCount == expectedHits,
    };
}

double renderOnce(QPdfDocument& document, int page, const QSize& size, bool* ok)
{
    QElapsedTimer timer;
    timer.start();
    const QImage image = document.render(page, size);
    const double elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    *ok = !image.isNull() && image.size() == size;
    return elapsedMs;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_qt_pdf_bench"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Atlas N2 repeated Qt PDF performance probe"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption fixtureIdOption(QStringLiteral("fixture-id"), QStringLiteral("Fixture identifier."), QStringLiteral("id"));
    const QCommandLineOption queryOption(QStringLiteral("query"), QStringLiteral("Known-hit Unicode query."), QStringLiteral("text"));
    const QCommandLineOption expectedHitsOption(QStringLiteral("expected-hits"), QStringLiteral("Expected exact query hit count."), QStringLiteral("count"));
    const QCommandLineOption iterationsOption(QStringLiteral("iterations"), QStringLiteral("Measured warm iterations per operation."), QStringLiteral("count"), QStringLiteral("31"));
    const QCommandLineOption warmupsOption(QStringLiteral("warmups"), QStringLiteral("Excluded warm-up iterations per operation."), QStringLiteral("count"), QStringLiteral("3"));
    const QCommandLineOption timeoutOption(QStringLiteral("search-timeout-ms"), QStringLiteral("Per-search timeout."), QStringLiteral("milliseconds"), QStringLiteral("5000"));
    const QCommandLineOption renderPageOption(QStringLiteral("render-page"), QStringLiteral("Zero-based render page."), QStringLiteral("page"), QStringLiteral("0"));
    const QCommandLineOption renderWidthOption(QStringLiteral("render-width"), QStringLiteral("Render width in pixels."), QStringLiteral("pixels"), QStringLiteral("612"));
    const QCommandLineOption renderHeightOption(QStringLiteral("render-height"), QStringLiteral("Render height in pixels."), QStringLiteral("pixels"), QStringLiteral("792"));

    parser.addOption(fixtureIdOption);
    parser.addOption(queryOption);
    parser.addOption(expectedHitsOption);
    parser.addOption(iterationsOption);
    parser.addOption(warmupsOption);
    parser.addOption(timeoutOption);
    parser.addOption(renderPageOption);
    parser.addOption(renderWidthOption);
    parser.addOption(renderHeightOption);
    parser.addPositionalArgument(QStringLiteral("pdf"), QStringLiteral("PDF fixture path."));
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.size() != 1 || !parser.isSet(queryOption) || !parser.isSet(expectedHitsOption)) {
        parser.showHelp(64);
    }

    QJsonArray failures;
    const auto iterations = parseInt(parser.value(iterationsOption));
    const auto warmups = parseInt(parser.value(warmupsOption));
    const auto expectedHits = parseInt(parser.value(expectedHitsOption));
    const auto timeoutMs = parseInt(parser.value(timeoutOption));
    const auto renderPage = parseInt(parser.value(renderPageOption));
    const auto renderWidth = parseInt(parser.value(renderWidthOption));
    const auto renderHeight = parseInt(parser.value(renderHeightOption));

    if (!iterations.has_value() || *iterations < 5 || !warmups.has_value() || *warmups < 0
        || !expectedHits.has_value() || *expectedHits <= 0 || !timeoutMs.has_value() || *timeoutMs <= 0
        || !renderPage.has_value() || *renderPage < 0 || !renderWidth.has_value() || *renderWidth <= 0
        || !renderHeight.has_value() || *renderHeight <= 0) {
        addFailure(failures, QStringLiteral("invalid-benchmark-arguments"));
    }

    const QString pdfPath = positional.constFirst();
    QFile file(pdfPath);
    QByteArray pdfBytes;
    if (!file.open(QIODevice::ReadOnly)) {
        addFailure(failures, QStringLiteral("fixture-open-failed"));
    } else {
        pdfBytes = file.readAll();
        file.close();
    }

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-bench.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("fixture_id"), parser.value(fixtureIdOption).isEmpty() ? QFileInfo(pdfPath).completeBaseName() : parser.value(fixtureIdOption));
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());
    result.insert(QStringLiteral("query_utf8_sha256"), sha256(parser.value(queryOption).toUtf8()));
    result.insert(QStringLiteral("iterations"), iterations.value_or(0));
    result.insert(QStringLiteral("warmups"), warmups.value_or(0));
    result.insert(QStringLiteral("percentile_method"), QStringLiteral("nearest-rank"));
    result.insert(QStringLiteral("open_input"), QStringLiteral("in-memory-bytes-via-QBuffer"));
    result.insert(QStringLiteral("search_completion"), QStringLiteral("known-expected-hit-count"));

    if (!failures.isEmpty()) {
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("passed"), false);
        const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
        fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
        return 2;
    }

    const atlas::bench::MemorySnapshot memoryBefore = atlas::bench::memorySnapshot();

    bool openOk = false;
    const double firstOpenMs = openOnce(pdfBytes, &openOk);
    if (!openOk) {
        addFailure(failures, QStringLiteral("first-open-failed"));
    }
    for (int i = 0; i < *warmups; ++i) {
        (void)openOnce(pdfBytes, &openOk);
        if (!openOk) {
            addFailure(failures, QStringLiteral("open-warmup-failed"));
            break;
        }
    }
    std::vector<double> openSamples;
    openSamples.reserve(static_cast<std::size_t>(*iterations));
    for (int i = 0; i < *iterations; ++i) {
        const double value = openOnce(pdfBytes, &openOk);
        if (!openOk) {
            addFailure(failures, QStringLiteral("open-sample-failed"));
            break;
        }
        openSamples.push_back(value);
    }

    QBuffer persistentBuffer;
    persistentBuffer.setData(pdfBytes);
    persistentBuffer.open(QIODevice::ReadOnly);
    QPdfDocument document;
    document.load(&persistentBuffer);
    if (!awaitDocumentReady(document)) {
        addFailure(failures, QStringLiteral("persistent-load-failed"));
    }
    if (*renderPage >= document.pageCount()) {
        addFailure(failures, QStringLiteral("render-page-out-of-range"));
    }

    qsizetype extractedLength = 0;
    const double firstExtractMs = extractAllPages(document, &extractedLength);
    if (extractedLength <= 0) {
        addFailure(failures, QStringLiteral("empty-extraction"));
    }
    for (int i = 0; i < *warmups; ++i) {
        (void)extractAllPages(document, &extractedLength);
    }
    std::vector<double> extractSamples;
    extractSamples.reserve(static_cast<std::size_t>(*iterations));
    for (int i = 0; i < *iterations; ++i) {
        extractSamples.push_back(extractAllPages(document, &extractedLength));
    }

    const SearchTiming firstSearch = searchOnce(document, parser.value(queryOption), *expectedHits, *timeoutMs);
    if (!firstSearch.completed) {
        addFailure(failures, QStringLiteral("first-search-hit-count-mismatch:%1").arg(firstSearch.hitCount));
    }
    for (int i = 0; i < *warmups; ++i) {
        const SearchTiming warm = searchOnce(document, parser.value(queryOption), *expectedHits, *timeoutMs);
        if (!warm.completed) {
            addFailure(failures, QStringLiteral("search-warmup-hit-count-mismatch:%1").arg(warm.hitCount));
            break;
        }
    }
    std::vector<double> searchSamples;
    searchSamples.reserve(static_cast<std::size_t>(*iterations));
    for (int i = 0; i < *iterations; ++i) {
        const SearchTiming sample = searchOnce(document, parser.value(queryOption), *expectedHits, *timeoutMs);
        if (!sample.completed) {
            addFailure(failures, QStringLiteral("search-sample-hit-count-mismatch:%1").arg(sample.hitCount));
            break;
        }
        searchSamples.push_back(sample.elapsedMs);
    }

    const QSize renderSize(*renderWidth, *renderHeight);
    bool renderOk = false;
    const double firstRenderMs = renderOnce(document, *renderPage, renderSize, &renderOk);
    if (!renderOk) {
        addFailure(failures, QStringLiteral("first-render-failed"));
    }
    for (int i = 0; i < *warmups; ++i) {
        (void)renderOnce(document, *renderPage, renderSize, &renderOk);
        if (!renderOk) {
            addFailure(failures, QStringLiteral("render-warmup-failed"));
            break;
        }
    }
    std::vector<double> renderSamples;
    renderSamples.reserve(static_cast<std::size_t>(*iterations));
    for (int i = 0; i < *iterations; ++i) {
        const double value = renderOnce(document, *renderPage, renderSize, &renderOk);
        if (!renderOk) {
            addFailure(failures, QStringLiteral("render-sample-failed"));
            break;
        }
        renderSamples.push_back(value);
    }

    const atlas::bench::MemorySnapshot memoryAfter = atlas::bench::memorySnapshot();

    result.insert(QStringLiteral("open"), atlas::bench::metricSummary(firstOpenMs, openSamples));
    result.insert(QStringLiteral("extract_all_pages"), atlas::bench::metricSummary(firstExtractMs, extractSamples));
    result.insert(QStringLiteral("search"), atlas::bench::metricSummary(firstSearch.elapsedMs, searchSamples));
    result.insert(QStringLiteral("render"), atlas::bench::metricSummary(firstRenderMs, renderSamples));
    result.insert(QStringLiteral("memory"), atlas::bench::memoryEvidence(memoryBefore, memoryAfter));
    result.insert(QStringLiteral("extracted_utf16_length"), static_cast<qint64>(extractedLength));
    result.insert(QStringLiteral("expected_search_hits"), *expectedHits);
    result.insert(QStringLiteral("render_page"), *renderPage);
    result.insert(QStringLiteral("render_width"), *renderWidth);
    result.insert(QStringLiteral("render_height"), *renderHeight);
    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty()
        && openSamples.size() == static_cast<std::size_t>(*iterations)
        && extractSamples.size() == static_cast<std::size_t>(*iterations)
        && searchSamples.size() == static_cast<std::size_t>(*iterations)
        && renderSamples.size() == static_cast<std::size_t>(*iterations));

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
    fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
    return result.value(QStringLiteral("passed")).toBool() ? 0 : 3;
}