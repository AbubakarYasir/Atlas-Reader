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

#include <fpdf_doc.h>
#include <fpdf_text.h>
#include <fpdfview.h>

#include <cstddef>
#include <cstdio>
#include <optional>
#include <unordered_set>
#include <vector>

namespace {

QString sha256(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

std::optional<int> parseInt(const QString& value)
{
    bool ok = false;
    const int parsed = value.toInt(&ok);
    return ok ? std::optional<int>(parsed) : std::nullopt;
}

QList<int> parseIntList(const QString& value, bool* ok)
{
    QList<int> result;
    *ok = true;
    if (value.isEmpty()) {
        return result;
    }

    const QStringList parts = value.split(QLatin1Char(','), Qt::KeepEmptyParts);
    for (const QString& part : parts) {
        const auto parsed = parseInt(part);
        if (!parsed.has_value()) {
            *ok = false;
            return {};
        }
        result.append(*parsed);
    }
    return result;
}

void addFailure(QJsonArray& failures, const QString& message)
{
    failures.append(message);
}

QString pdfiumErrorName(unsigned long error)
{
    switch (error) {
    case FPDF_ERR_SUCCESS:
        return QStringLiteral("success");
    case FPDF_ERR_UNKNOWN:
        return QStringLiteral("unknown");
    case FPDF_ERR_FILE:
        return QStringLiteral("file");
    case FPDF_ERR_FORMAT:
        return QStringLiteral("format");
    case FPDF_ERR_PASSWORD:
        return QStringLiteral("password");
    case FPDF_ERR_SECURITY:
        return QStringLiteral("security");
    case FPDF_ERR_PAGE:
        return QStringLiteral("page");
    default:
        return QStringLiteral("unmapped");
    }
}

QString pageLabel(FPDF_DOCUMENT document, int pageIndex)
{
    const unsigned long bytesRequired = FPDF_GetPageLabel(document, pageIndex, nullptr, 0);
    if (bytesRequired < 2) {
        return QString::number(pageIndex + 1);
    }

    std::vector<unsigned char> buffer(bytesRequired);
    const unsigned long written = FPDF_GetPageLabel(
        document, pageIndex, buffer.data(), static_cast<unsigned long>(buffer.size()));
    if (written < 2 || written > buffer.size()) {
        return {};
    }

    const auto* utf16 = reinterpret_cast<const char16_t*>(buffer.data());
    const qsizetype codeUnits = static_cast<qsizetype>((written / 2) - 1);
    return QString::fromUtf16(utf16, codeUnits);
}

QString pageText(FPDF_PAGE page, double* elapsedMs, QJsonArray& failures)
{
    QElapsedTimer timer;
    timer.start();

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    if (textPage == nullptr) {
        *elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
        addFailure(failures, QStringLiteral("text-page-load-failed"));
        return {};
    }

    const int count = FPDFText_CountChars(textPage);
    if (count < 0) {
        FPDFText_ClosePage(textPage);
        *elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
        addFailure(failures, QStringLiteral("text-char-count-failed"));
        return {};
    }

    std::vector<unsigned short> buffer(static_cast<std::size_t>(count) + 1U, 0);
    const int written = FPDFText_GetText(textPage, 0, count, buffer.data());
    FPDFText_ClosePage(textPage);

    *elapsedMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    if (written <= 0) {
        return {};
    }

    const auto* utf16 = reinterpret_cast<const char16_t*>(buffer.data());
    const int textCodeUnits = written > 1 ? written - 1 : 0;
    return QString::fromUtf16(utf16, static_cast<qsizetype>(textCodeUnits));
}

QString bookmarkTitle(FPDF_BOOKMARK bookmark)
{
    const unsigned long bytesRequired = FPDFBookmark_GetTitle(bookmark, nullptr, 0);
    if (bytesRequired < 2) {
        return {};
    }

    std::vector<unsigned char> buffer(bytesRequired);
    const unsigned long written = FPDFBookmark_GetTitle(
        bookmark, buffer.data(), static_cast<unsigned long>(buffer.size()));
    if (written < 2 || written > buffer.size()) {
        return {};
    }

    const auto* utf16 = reinterpret_cast<const char16_t*>(buffer.data());
    return QString::fromUtf16(utf16, static_cast<qsizetype>((written / 2) - 1));
}

FPDF_DEST actionDestination(FPDF_DOCUMENT document, FPDF_ACTION action)
{
    if (action == nullptr || FPDFAction_GetType(action) != PDFACTION_GOTO) {
        return nullptr;
    }
    return FPDFAction_GetDest(document, action);
}

FPDF_DEST bookmarkDestination(FPDF_DOCUMENT document, FPDF_BOOKMARK bookmark)
{
    FPDF_DEST dest = FPDFBookmark_GetDest(document, bookmark);
    if (dest != nullptr) {
        return dest;
    }
    return actionDestination(document, FPDFBookmark_GetAction(bookmark));
}

QString actionUri(FPDF_DOCUMENT document, FPDF_ACTION action)
{
    if (action == nullptr || FPDFAction_GetType(action) != PDFACTION_URI) {
        return {};
    }

    const unsigned long bytesRequired = FPDFAction_GetURIPath(document, action, nullptr, 0);
    if (bytesRequired < 1) {
        return {};
    }

    QByteArray buffer(static_cast<qsizetype>(bytesRequired), '\0');
    const unsigned long written = FPDFAction_GetURIPath(
        document, action, buffer.data(), static_cast<unsigned long>(buffer.size()));
    if (written < 1 || written > static_cast<unsigned long>(buffer.size())) {
        return {};
    }

    if (!buffer.isEmpty() && buffer.at(buffer.size() - 1) == '\0') {
        buffer.chop(1);
    }
    return QString::fromUtf8(buffer);
}

void appendDestination(QJsonObject& item, FPDF_DOCUMENT document, FPDF_DEST dest)
{
    const int page = dest != nullptr ? FPDFDest_GetDestPageIndex(document, dest) : -1;
    item.insert(QStringLiteral("destination_page"), page);

    FPDF_BOOL hasX = false;
    FPDF_BOOL hasY = false;
    FPDF_BOOL hasZoom = false;
    FS_FLOAT x = 0;
    FS_FLOAT y = 0;
    FS_FLOAT zoom = 0;
    if (dest != nullptr
        && FPDFDest_GetLocationInPage(dest, &hasX, &hasY, &hasZoom, &x, &y, &zoom)) {
        item.insert(QStringLiteral("has_location_x"), static_cast<bool>(hasX));
        item.insert(QStringLiteral("has_location_y"), static_cast<bool>(hasY));
        item.insert(QStringLiteral("has_zoom"), static_cast<bool>(hasZoom));
        if (hasX) {
            item.insert(QStringLiteral("location_x"), static_cast<double>(x));
        }
        if (hasY) {
            item.insert(QStringLiteral("location_y"), static_cast<double>(y));
        }
        if (hasZoom) {
            item.insert(QStringLiteral("zoom"), static_cast<double>(zoom));
        }
    }
}

void collectBookmarks(
    FPDF_DOCUMENT document,
    FPDF_BOOKMARK parent,
    int depth,
    QJsonArray& output,
    QStringList& titles,
    QList<int>& depths,
    QList<int>& pages,
    std::unordered_set<FPDF_BOOKMARK>& visited,
    QJsonArray& failures)
{
    for (FPDF_BOOKMARK bookmark = FPDFBookmark_GetFirstChild(document, parent);
         bookmark != nullptr;
         bookmark = FPDFBookmark_GetNextSibling(document, bookmark)) {
        if (!visited.insert(bookmark).second) {
            addFailure(failures, QStringLiteral("outline-cycle-detected"));
            return;
        }

        const QString title = bookmarkTitle(bookmark);
        FPDF_DEST dest = bookmarkDestination(document, bookmark);
        const int page = dest != nullptr ? FPDFDest_GetDestPageIndex(document, dest) : -1;

        QJsonObject item;
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("depth"), depth);
        appendDestination(item, document, dest);
        output.append(item);

        titles.append(title);
        depths.append(depth);
        pages.append(page);

        collectBookmarks(document, bookmark, depth + 1, output, titles, depths, pages, visited, failures);
    }
}

QJsonArray collectLinks(
    FPDF_DOCUMENT document,
    int pageCount,
    QList<int>& internalPages,
    QStringList& externalUris,
    QJsonArray& failures)
{
    QJsonArray output;

    for (int sourcePage = 0; sourcePage < pageCount; ++sourcePage) {
        FPDF_PAGE page = FPDF_LoadPage(document, sourcePage);
        if (page == nullptr) {
            addFailure(failures, QStringLiteral("link-page-load-failed:%1").arg(sourcePage));
            continue;
        }

        int position = 0;
        FPDF_LINK link = nullptr;
        while (FPDFLink_Enumerate(page, &position, &link)) {
            QJsonObject item;
            item.insert(QStringLiteral("source_page"), sourcePage);

            FS_RECTF rect{};
            if (FPDFLink_GetAnnotRect(link, &rect)) {
                QJsonObject rectangle;
                rectangle.insert(QStringLiteral("left"), static_cast<double>(rect.left));
                rectangle.insert(QStringLiteral("top"), static_cast<double>(rect.top));
                rectangle.insert(QStringLiteral("right"), static_cast<double>(rect.right));
                rectangle.insert(QStringLiteral("bottom"), static_cast<double>(rect.bottom));
                item.insert(QStringLiteral("rectangle"), rectangle);
            }

            FPDF_DEST dest = FPDFLink_GetDest(document, link);
            FPDF_ACTION action = FPDFLink_GetAction(link);
            if (dest == nullptr) {
                dest = actionDestination(document, action);
            }
            appendDestination(item, document, dest);

            const int destinationPage = dest != nullptr ? FPDFDest_GetDestPageIndex(document, dest) : -1;
            const QString uri = actionUri(document, action);
            item.insert(QStringLiteral("url"), uri);
            output.append(item);

            if (destinationPage >= 0 && uri.isEmpty()) {
                internalPages.append(destinationPage);
            }
            if (!uri.isEmpty()) {
                externalUris.append(uri);
            }
        }

        FPDF_ClosePage(page);
    }

    return output;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_pdfium_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Atlas N2 focused PDFium qualification probe"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption fixtureIdOption(
        QStringLiteral("fixture-id"),
        QStringLiteral("Sanitized fixture identifier recorded in probe output."),
        QStringLiteral("id"));
    const QCommandLineOption expectPagesOption(
        QStringLiteral("expect-pages"),
        QStringLiteral("Fail if the document page count differs."),
        QStringLiteral("count"));
    const QCommandLineOption expectLabelsOption(
        QStringLiteral("expect-labels"),
        QStringLiteral("Comma-separated expected display page labels."),
        QStringLiteral("labels"));
    const QCommandLineOption expectTextContainsOption(
        QStringLiteral("expect-text-contains"),
        QStringLiteral("Fail unless concatenated extracted text contains this value."),
        QStringLiteral("text"));
    const QCommandLineOption expectOutlineCountOption(
        QStringLiteral("expect-outline-count"),
        QStringLiteral("Expected flattened outline item count."),
        QStringLiteral("count"));
    const QCommandLineOption expectOutlineTitlesOption(
        QStringLiteral("expect-outline-titles"),
        QStringLiteral("Pipe-separated expected flattened outline titles."),
        QStringLiteral("titles"));
    const QCommandLineOption expectOutlineDepthsOption(
        QStringLiteral("expect-outline-depths"),
        QStringLiteral("Comma-separated expected outline tree depths."),
        QStringLiteral("depths"));
    const QCommandLineOption expectOutlinePagesOption(
        QStringLiteral("expect-outline-pages"),
        QStringLiteral("Comma-separated expected zero-based outline destination pages."),
        QStringLiteral("pages"));
    const QCommandLineOption expectLinkCountOption(
        QStringLiteral("expect-link-count"),
        QStringLiteral("Expected total link count across the document."),
        QStringLiteral("count"));
    const QCommandLineOption expectInternalLinkPageOption(
        QStringLiteral("expect-internal-link-page"),
        QStringLiteral("Expected zero-based internal link destination page."),
        QStringLiteral("page"));
    const QCommandLineOption expectExternalUriOption(
        QStringLiteral("expect-external-uri"),
        QStringLiteral("Expected external URI link."),
        QStringLiteral("uri"));
    const QCommandLineOption renderPageOption(
        QStringLiteral("render-page"),
        QStringLiteral("Render one zero-based page and record timing/hash."),
        QStringLiteral("index"));
    const QCommandLineOption renderWidthOption(
        QStringLiteral("render-width"),
        QStringLiteral("Requested render width in pixels."),
        QStringLiteral("pixels"),
        QStringLiteral("800"));
    const QCommandLineOption renderHeightOption(
        QStringLiteral("render-height"),
        QStringLiteral("Requested render height in pixels."),
        QStringLiteral("pixels"),
        QStringLiteral("1000"));

    parser.addOption(fixtureIdOption);
    parser.addOption(expectPagesOption);
    parser.addOption(expectLabelsOption);
    parser.addOption(expectTextContainsOption);
    parser.addOption(expectOutlineCountOption);
    parser.addOption(expectOutlineTitlesOption);
    parser.addOption(expectOutlineDepthsOption);
    parser.addOption(expectOutlinePagesOption);
    parser.addOption(expectLinkCountOption);
    parser.addOption(expectInternalLinkPageOption);
    parser.addOption(expectExternalUriOption);
    parser.addOption(renderPageOption);
    parser.addOption(renderWidthOption);
    parser.addOption(renderHeightOption);
    parser.addPositionalArgument(QStringLiteral("pdf"), QStringLiteral("PDF fixture path."));
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.size() != 1) {
        parser.showHelp(64);
    }

    const QString pdfPath = positional.constFirst();
    const QString fixtureId = parser.value(fixtureIdOption).isEmpty()
        ? QFileInfo(pdfPath).completeBaseName()
        : parser.value(fixtureIdOption);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-probe.v2"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("pdfium_pin"), QStringLiteral(ATLAS_PDFIUM_PIN));
    result.insert(QStringLiteral("pdfium_version"), QStringLiteral(ATLAS_PDFIUM_VERSION));
    result.insert(QStringLiteral("call_model"), QStringLiteral("serialized-single-thread"));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());

    QJsonArray failures;

    QFile file(pdfPath);
    if (!file.open(QIODevice::ReadOnly)) {
        addFailure(failures, QStringLiteral("fixture-open-failed"));
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("passed"), false);
        const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
        fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
        return 2;
    }
    const QByteArray pdfBytes = file.readAll();
    file.close();

    FPDF_InitLibrary();

    QElapsedTimer openTimer;
    openTimer.start();
    FPDF_DOCUMENT document = FPDF_LoadMemDocument64(
        pdfBytes.constData(), static_cast<std::size_t>(pdfBytes.size()), nullptr);
    const qint64 openElapsedNs = openTimer.nsecsElapsed();

    result.insert(QStringLiteral("open_ms"), static_cast<double>(openElapsedNs) / 1'000'000.0);

    if (document == nullptr) {
        const unsigned long error = FPDF_GetLastError();
        result.insert(QStringLiteral("load_error"), pdfiumErrorName(error));
        result.insert(QStringLiteral("load_error_code"), static_cast<qint64>(error));
        addFailure(failures, QStringLiteral("document-load-failed:%1").arg(pdfiumErrorName(error)));
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("passed"), false);
        FPDF_DestroyLibrary();
        const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
        fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
        return 2;
    }

    result.insert(QStringLiteral("load_error"), QStringLiteral("success"));
    result.insert(QStringLiteral("load_error_code"), 0);

    const int pageCount = FPDF_GetPageCount(document);
    result.insert(QStringLiteral("page_count"), pageCount);

    if (parser.isSet(expectPagesOption)) {
        const auto expected = parseInt(parser.value(expectPagesOption));
        if (!expected.has_value()) {
            addFailure(failures, QStringLiteral("invalid-expect-pages"));
        } else if (pageCount != *expected) {
            addFailure(failures, QStringLiteral("page-count:%1!=%2").arg(pageCount).arg(*expected));
        }
    }

    QJsonArray pages;
    QStringList extractedTexts;
    QStringList labels;

    for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        FPDF_PAGE pageHandle = FPDF_LoadPage(document, pageIndex);
        if (pageHandle == nullptr) {
            addFailure(failures, QStringLiteral("page-load-failed:%1").arg(pageIndex));
            continue;
        }

        QJsonObject page;
        page.insert(QStringLiteral("index"), pageIndex);

        const QString label = pageLabel(document, pageIndex);
        labels.append(label);
        page.insert(QStringLiteral("label"), label);
        page.insert(QStringLiteral("width_points"), FPDF_GetPageWidthF(pageHandle));
        page.insert(QStringLiteral("height_points"), FPDF_GetPageHeightF(pageHandle));

        double textMs = 0.0;
        const QString text = pageText(pageHandle, &textMs, failures);
        extractedTexts.append(text);
        page.insert(QStringLiteral("text_ms"), textMs);
        page.insert(QStringLiteral("text_utf16_length"), text.size());
        page.insert(QStringLiteral("text_utf8_sha256"), sha256(text.toUtf8()));
        pages.append(page);

        FPDF_ClosePage(pageHandle);
    }

    result.insert(QStringLiteral("pages"), pages);

    if (parser.isSet(expectLabelsOption)) {
        const QStringList expectedLabels = parser.value(expectLabelsOption).split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (labels != expectedLabels) {
            addFailure(
                failures,
                QStringLiteral("page-labels:%1!=%2")
                    .arg(labels.join(QLatin1Char(',')), expectedLabels.join(QLatin1Char(','))));
        }
    }

    if (parser.isSet(expectTextContainsOption)) {
        const QString allText = extractedTexts.join(QLatin1Char('\n'));
        if (!allText.contains(parser.value(expectTextContainsOption))) {
            addFailure(failures, QStringLiteral("expected-text-not-found"));
        }
    }

    QJsonArray outlines;
    QStringList outlineTitles;
    QList<int> outlineDepths;
    QList<int> outlinePages;
    std::unordered_set<FPDF_BOOKMARK> visitedBookmarks;
    collectBookmarks(
        document,
        nullptr,
        0,
        outlines,
        outlineTitles,
        outlineDepths,
        outlinePages,
        visitedBookmarks,
        failures);
    result.insert(QStringLiteral("outlines"), outlines);

    if (parser.isSet(expectOutlineCountOption)) {
        const auto expected = parseInt(parser.value(expectOutlineCountOption));
        if (!expected.has_value()) {
            addFailure(failures, QStringLiteral("invalid-expect-outline-count"));
        } else if (outlines.size() != *expected) {
            addFailure(failures, QStringLiteral("outline-count:%1!=%2").arg(outlines.size()).arg(*expected));
        }
    }

    if (parser.isSet(expectOutlineTitlesOption)) {
        const QStringList expected = parser.value(expectOutlineTitlesOption).split(QLatin1Char('|'), Qt::KeepEmptyParts);
        if (outlineTitles != expected) {
            addFailure(failures, QStringLiteral("outline-titles-mismatch"));
        }
    }

    if (parser.isSet(expectOutlineDepthsOption)) {
        bool ok = false;
        const QList<int> expected = parseIntList(parser.value(expectOutlineDepthsOption), &ok);
        if (!ok) {
            addFailure(failures, QStringLiteral("invalid-expect-outline-depths"));
        } else if (outlineDepths != expected) {
            addFailure(failures, QStringLiteral("outline-depths-mismatch"));
        }
    }

    if (parser.isSet(expectOutlinePagesOption)) {
        bool ok = false;
        const QList<int> expected = parseIntList(parser.value(expectOutlinePagesOption), &ok);
        if (!ok) {
            addFailure(failures, QStringLiteral("invalid-expect-outline-pages"));
        } else if (outlinePages != expected) {
            addFailure(failures, QStringLiteral("outline-pages-mismatch"));
        }
    }

    QList<int> internalLinkPages;
    QStringList externalUris;
    const QJsonArray links = collectLinks(document, pageCount, internalLinkPages, externalUris, failures);
    result.insert(QStringLiteral("links"), links);

    if (parser.isSet(expectLinkCountOption)) {
        const auto expected = parseInt(parser.value(expectLinkCountOption));
        if (!expected.has_value()) {
            addFailure(failures, QStringLiteral("invalid-expect-link-count"));
        } else if (links.size() != *expected) {
            addFailure(failures, QStringLiteral("link-count:%1!=%2").arg(links.size()).arg(*expected));
        }
    }

    if (parser.isSet(expectInternalLinkPageOption)) {
        const auto expected = parseInt(parser.value(expectInternalLinkPageOption));
        if (!expected.has_value()) {
            addFailure(failures, QStringLiteral("invalid-expect-internal-link-page"));
        } else if (!internalLinkPages.contains(*expected)) {
            addFailure(failures, QStringLiteral("internal-link-destination-missing:%1").arg(*expected));
        }
    }

    if (parser.isSet(expectExternalUriOption)) {
        const QString expected = parser.value(expectExternalUriOption);
        if (!externalUris.contains(expected)) {
            addFailure(failures, QStringLiteral("external-uri-missing:%1").arg(expected));
        }
    }

    if (parser.isSet(renderPageOption)) {
        const auto pageIndex = parseInt(parser.value(renderPageOption));
        const auto width = parseInt(parser.value(renderWidthOption));
        const auto height = parseInt(parser.value(renderHeightOption));

        if (!pageIndex.has_value() || !width.has_value() || !height.has_value()
            || *pageIndex < 0 || *pageIndex >= pageCount || *width <= 0 || *height <= 0) {
            addFailure(failures, QStringLiteral("invalid-render-request"));
        } else {
            FPDF_PAGE pageHandle = FPDF_LoadPage(document, *pageIndex);
            FPDF_BITMAP bitmap = FPDFBitmap_Create(*width, *height, 1);
            if (pageHandle == nullptr || bitmap == nullptr) {
                addFailure(failures, QStringLiteral("render-allocation-failed"));
            } else {
                FPDFBitmap_FillRect(bitmap, 0, 0, *width, *height, 0xFFFFFFFFU);

                QElapsedTimer renderTimer;
                renderTimer.start();
                FPDF_RenderPageBitmap(bitmap, pageHandle, 0, 0, *width, *height, 0, 0);
                const qint64 renderElapsedNs = renderTimer.nsecsElapsed();

                const int stride = FPDFBitmap_GetStride(bitmap);
                const auto* pixels = static_cast<const char*>(FPDFBitmap_GetBuffer(bitmap));
                const qsizetype byteCount = static_cast<qsizetype>(stride) * *height;
                const QByteArray pixelBytes(pixels, byteCount);

                QJsonObject render;
                render.insert(QStringLiteral("page_index"), *pageIndex);
                render.insert(QStringLiteral("requested_width"), *width);
                render.insert(QStringLiteral("requested_height"), *height);
                render.insert(QStringLiteral("actual_width"), FPDFBitmap_GetWidth(bitmap));
                render.insert(QStringLiteral("actual_height"), FPDFBitmap_GetHeight(bitmap));
                render.insert(QStringLiteral("stride"), stride);
                render.insert(QStringLiteral("render_ms"), static_cast<double>(renderElapsedNs) / 1'000'000.0);
                render.insert(QStringLiteral("pixel_sha256"), sha256(pixelBytes));
                result.insert(QStringLiteral("render"), render);
            }

            if (bitmap != nullptr) {
                FPDFBitmap_Destroy(bitmap);
            }
            if (pageHandle != nullptr) {
                FPDF_ClosePage(pageHandle);
            }
        }
    }

    FPDF_CloseDocument(document);
    FPDF_DestroyLibrary();

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
    fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
    return failures.isEmpty() ? 0 : 3;
}
