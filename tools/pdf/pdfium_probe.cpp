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

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <optional>
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
    return QString::fromUtf16(utf16, static_cast<qsizetype>(std::max(0, written - 1)));
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
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-probe.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("pdfium_pin"), QStringLiteral(ATLAS_PDFIUM_PIN));
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
        pdfBytes.constData(), static_cast<size_t>(pdfBytes.size()), nullptr);
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
            addFailure(
                failures,
                QStringLiteral("page-count:%1!=%2").arg(pageCount).arg(*expected));
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
        const QStringList expectedLabels = parser.value(expectLabelsOption).split(
            QLatin1Char(','), Qt::KeepEmptyParts);
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
                render.insert(
                    QStringLiteral("render_ms"),
                    static_cast<double>(renderElapsedNs) / 1'000'000.0);
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
