#include "bench_common.h"

#include <QByteArray>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <fpdf_text.h>
#include <fpdfview.h>

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

std::vector<unsigned short> pdfiumWideString(const QString& value)
{
    std::vector<unsigned short> result(static_cast<std::size_t>(value.size()) + 1U, 0);
    for (qsizetype i = 0; i < value.size(); ++i) {
        result[static_cast<std::size_t>(i)] = value.at(i).unicode();
    }
    return result;
}

double openOnce(const QByteArray& pdfBytes, bool* ok)
{
    QElapsedTimer timer;
    timer.start();
    FPDF_DOCUMENT document = FPDF_LoadMemDocument64(
        pdfBytes.constData(), static_cast<std::size_t>(pdfBytes.size()), nullptr);
    const double elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    *ok = document != nullptr && FPDF_GetPageCount(document) > 0;
    if (document != nullptr) {
        FPDF_CloseDocument(document);
    }
    return elapsedMs;
}

double extractAllPages(FPDF_DOCUMENT document, qint64* textLength, bool* ok)
{
    qint64 totalLength = 0;
    bool valid = true;
    QElapsedTimer timer;
    timer.start();

    const int pageCount = FPDF_GetPageCount(document);
    for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        FPDF_PAGE page = FPDF_LoadPage(document, pageIndex);
        if (page == nullptr) {
            valid = false;
            continue;
        }
        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
        if (textPage == nullptr) {
            valid = false;
            FPDF_ClosePage(page);
            continue;
        }

        const int count = FPDFText_CountChars(textPage);
        if (count < 0) {
            valid = false;
        } else {
            std::vector<unsigned short> buffer(static_cast<std::size_t>(count) + 1U, 0);
            const int written = FPDFText_GetText(textPage, 0, count, buffer.data());
            if (count > 0 && written <= 0) {
                valid = false;
            }
            totalLength += count;
        }

        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);
    }

    const double elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    *textLength = totalLength;
    *ok = valid;
    return elapsedMs;
}

SearchTiming searchOnce(FPDF_DOCUMENT document, const std::vector<unsigned short>& query, int expectedHits)
{
    int hitCount = 0;
    bool valid = true;
    QElapsedTimer timer;
    timer.start();

    const int pageCount = FPDF_GetPageCount(document);
    for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        FPDF_PAGE page = FPDF_LoadPage(document, pageIndex);
        if (page == nullptr) {
            valid = false;
            continue;
        }
        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
        if (textPage == nullptr) {
            valid = false;
            FPDF_ClosePage(page);
            continue;
        }
        FPDF_SCHHANDLE search = FPDFText_FindStart(textPage, query.data(), 0, 0);
        if (search == nullptr) {
            valid = false;
        } else {
            while (FPDFText_FindNext(search)) {
                ++hitCount;
            }
            FPDFText_FindClose(search);
        }
        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);
    }

    return {
        static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0,
        hitCount,
        valid && hitCount == expectedHits,
    };
}

double renderOnce(FPDF_DOCUMENT document, int pageIndex, int width, int height, bool* ok)
{
    QElapsedTimer timer;
    timer.start();

    FPDF_PAGE page = FPDF_LoadPage(document, pageIndex);
    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    bool valid = page != nullptr && bitmap != nullptr;
    if (valid) {
        FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFFU);
        FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);
        valid = FPDFBitmap_GetBuffer(bitmap) != nullptr
            && FPDFBitmap_GetWidth(bitmap) == width
            && FPDFBitmap_GetHeight(bitmap) == height;
    }

    const double elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    if (bitmap != nullptr) {
        FPDFBitmap_Destroy(bitmap);
    }
    if (page != nullptr) {
        FPDF_ClosePage(page);
    }
    *ok = valid;
    return elapsedMs;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_pdfium_bench"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Atlas N2 repeated PDFium performance probe"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption fixtureIdOption(QStringLiteral("fixture-id"), QStringLiteral("Fixture identifier."), QStringLiteral("id"));
    const QCommandLineOption queryOption(QStringLiteral("query"), QStringLiteral("Known-hit Unicode query."), QStringLiteral("text"));
    const QCommandLineOption expectedHitsOption(QStringLiteral("expected-hits"), QStringLiteral("Expected exact query hit count."), QStringLiteral("count"));
    const QCommandLineOption iterationsOption(QStringLiteral("iterations"), QStringLiteral("Measured warm iterations per operation."), QStringLiteral("count"), QStringLiteral("31"));
    const QCommandLineOption warmupsOption(QStringLiteral("warmups"), QStringLiteral("Excluded warm-up iterations per operation."), QStringLiteral("count"), QStringLiteral("3"));
    const QCommandLineOption renderPageOption(QStringLiteral("render-page"), QStringLiteral("Zero-based render page."), QStringLiteral("page"), QStringLiteral("0"));
    const QCommandLineOption renderWidthOption(QStringLiteral("render-width"), QStringLiteral("Render width in pixels."), QStringLiteral("pixels"), QStringLiteral("612"));
    const QCommandLineOption renderHeightOption(QStringLiteral("render-height"), QStringLiteral("Render height in pixels."), QStringLiteral("pixels"), QStringLiteral("792"));

    parser.addOption(fixtureIdOption);
    parser.addOption(queryOption);
    parser.addOption(expectedHitsOption);
    parser.addOption(iterationsOption);
    parser.addOption(warmupsOption);
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
    const auto renderPage = parseInt(parser.value(renderPageOption));
    const auto renderWidth = parseInt(parser.value(renderWidthOption));
    const auto renderHeight = parseInt(parser.value(renderHeightOption));

    if (!iterations.has_value() || *iterations < 5 || !warmups.has_value() || *warmups < 0
        || !expectedHits.has_value() || *expectedHits <= 0 || !renderPage.has_value() || *renderPage < 0
        || !renderWidth.has_value() || *renderWidth <= 0 || !renderHeight.has_value() || *renderHeight <= 0) {
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
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-bench.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("pdfium_pin"), QStringLiteral(ATLAS_PDFIUM_PIN));
    result.insert(QStringLiteral("pdfium_version"), QStringLiteral(ATLAS_PDFIUM_VERSION));
    result.insert(QStringLiteral("call_model"), QStringLiteral("serialized-single-thread"));
    result.insert(QStringLiteral("fixture_id"), parser.value(fixtureIdOption).isEmpty() ? QFileInfo(pdfPath).completeBaseName() : parser.value(fixtureIdOption));
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());
    result.insert(QStringLiteral("query_utf8_sha256"), sha256(parser.value(queryOption).toUtf8()));
    result.insert(QStringLiteral("iterations"), iterations.value_or(0));
    result.insert(QStringLiteral("warmups"), warmups.value_or(0));
    result.insert(QStringLiteral("percentile_method"), QStringLiteral("nearest-rank"));
    result.insert(QStringLiteral("open_input"), QStringLiteral("in-memory-bytes"));
    result.insert(QStringLiteral("search_completion"), QStringLiteral("synchronous-full-document-known-hit-count"));

    if (!failures.isEmpty()) {
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("passed"), false);
        const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
        fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
        return 2;
    }

    FPDF_InitLibrary();
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

    FPDF_DOCUMENT document = FPDF_LoadMemDocument64(
        pdfBytes.constData(), static_cast<std::size_t>(pdfBytes.size()), nullptr);
    if (document == nullptr) {
        addFailure(failures, QStringLiteral("persistent-load-failed"));
    }
    if (document != nullptr && *renderPage >= FPDF_GetPageCount(document)) {
        addFailure(failures, QStringLiteral("render-page-out-of-range"));
    }

    qint64 extractedLength = 0;
    bool extractOk = false;
    const double firstExtractMs = document != nullptr
        ? extractAllPages(document, &extractedLength, &extractOk)
        : 0.0;
    if (!extractOk || extractedLength <= 0) {
        addFailure(failures, QStringLiteral("first-extraction-failed"));
    }
    for (int i = 0; document != nullptr && i < *warmups; ++i) {
        (void)extractAllPages(document, &extractedLength, &extractOk);
        if (!extractOk) {
            addFailure(failures, QStringLiteral("extraction-warmup-failed"));
            break;
        }
    }
    std::vector<double> extractSamples;
    extractSamples.reserve(static_cast<std::size_t>(*iterations));
    for (int i = 0; document != nullptr && i < *iterations; ++i) {
        const double value = extractAllPages(document, &extractedLength, &extractOk);
        if (!extractOk) {
            addFailure(failures, QStringLiteral("extraction-sample-failed"));
            break;
        }
        extractSamples.push_back(value);
    }

    const std::vector<unsigned short> queryWide = pdfiumWideString(parser.value(queryOption));
    const SearchTiming firstSearch = document != nullptr
        ? searchOnce(document, queryWide, *expectedHits)
        : SearchTiming{};
    if (!firstSearch.completed) {
        addFailure(failures, QStringLiteral("first-search-hit-count-mismatch:%1").arg(firstSearch.hitCount));
    }
    for (int i = 0; document != nullptr && i < *warmups; ++i) {
        const SearchTiming warm = searchOnce(document, queryWide, *expectedHits);
        if (!warm.completed) {
            addFailure(failures, QStringLiteral("search-warmup-hit-count-mismatch:%1").arg(warm.hitCount));
            break;
        }
    }
    std::vector<double> searchSamples;
    searchSamples.reserve(static_cast<std::size_t>(*iterations));
    for (int i = 0; document != nullptr && i < *iterations; ++i) {
        const SearchTiming sample = searchOnce(document, queryWide, *expectedHits);
        if (!sample.completed) {
            addFailure(failures, QStringLiteral("search-sample-hit-count-mismatch:%1").arg(sample.hitCount));
            break;
        }
        searchSamples.push_back(sample.elapsedMs);
    }

    bool renderOk = false;
    const double firstRenderMs = document != nullptr
        ? renderOnce(document, *renderPage, *renderWidth, *renderHeight, &renderOk)
        : 0.0;
    if (!renderOk) {
        addFailure(failures, QStringLiteral("first-render-failed"));
    }
    for (int i = 0; document != nullptr && i < *warmups; ++i) {
        (void)renderOnce(document, *renderPage, *renderWidth, *renderHeight, &renderOk);
        if (!renderOk) {
            addFailure(failures, QStringLiteral("render-warmup-failed"));
            break;
        }
    }
    std::vector<double> renderSamples;
    renderSamples.reserve(static_cast<std::size_t>(*iterations));
    for (int i = 0; document != nullptr && i < *iterations; ++i) {
        const double value = renderOnce(document, *renderPage, *renderWidth, *renderHeight, &renderOk);
        if (!renderOk) {
            addFailure(failures, QStringLiteral("render-sample-failed"));
            break;
        }
        renderSamples.push_back(value);
    }

    if (document != nullptr) {
        FPDF_CloseDocument(document);
    }
    const atlas::bench::MemorySnapshot memoryAfter = atlas::bench::memorySnapshot();
    FPDF_DestroyLibrary();

    result.insert(QStringLiteral("open"), atlas::bench::metricSummary(firstOpenMs, openSamples));
    result.insert(QStringLiteral("extract_all_pages"), atlas::bench::metricSummary(firstExtractMs, extractSamples));
    result.insert(QStringLiteral("search"), atlas::bench::metricSummary(firstSearch.elapsedMs, searchSamples));
    result.insert(QStringLiteral("render"), atlas::bench::metricSummary(firstRenderMs, renderSamples));
    result.insert(QStringLiteral("memory"), atlas::bench::memoryEvidence(memoryBefore, memoryAfter));
    result.insert(QStringLiteral("extracted_utf16_length"), extractedLength);
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
