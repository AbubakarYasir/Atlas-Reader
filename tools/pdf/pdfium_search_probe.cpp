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
#include <QString>
#include <QStringList>

#include <fpdf_text.h>
#include <fpdfview.h>

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

std::vector<unsigned short> pdfiumWideString(const QString& value)
{
    std::vector<unsigned short> result(static_cast<std::size_t>(value.size()) + 1U, 0);
    for (qsizetype i = 0; i < value.size(); ++i) {
        result[static_cast<std::size_t>(i)] = value.at(i).unicode();
    }
    return result;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_pdfium_search_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Atlas N2 focused PDFium search qualification probe"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption fixtureIdOption(
        QStringLiteral("fixture-id"),
        QStringLiteral("Sanitized fixture identifier recorded in probe output."),
        QStringLiteral("id"));
    const QCommandLineOption queryOption(
        QStringLiteral("query"),
        QStringLiteral("Unicode search query."),
        QStringLiteral("text"));
    const QCommandLineOption expectCountOption(
        QStringLiteral("expect-count"),
        QStringLiteral("Expected total search hit count."),
        QStringLiteral("count"));
    const QCommandLineOption expectFirstPageOption(
        QStringLiteral("expect-first-page"),
        QStringLiteral("Expected zero-based page of the first hit."),
        QStringLiteral("page"));
    const QCommandLineOption expectFirstOrdinalOption(
        QStringLiteral("expect-first-ordinal"),
        QStringLiteral("Expected zero-based normalized ordinal of the first hit on its page."),
        QStringLiteral("ordinal"),
        QStringLiteral("0"));

    parser.addOption(fixtureIdOption);
    parser.addOption(queryOption);
    parser.addOption(expectCountOption);
    parser.addOption(expectFirstPageOption);
    parser.addOption(expectFirstOrdinalOption);
    parser.addPositionalArgument(QStringLiteral("pdf"), QStringLiteral("PDF fixture path."));
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.size() != 1 || !parser.isSet(queryOption)) {
        parser.showHelp(64);
    }

    const QString pdfPath = positional.constFirst();
    const QString fixtureId = parser.value(fixtureIdOption).isEmpty()
        ? QFileInfo(pdfPath).completeBaseName()
        : parser.value(fixtureIdOption);
    const QString query = parser.value(queryOption);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-search-probe.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("pdfium_pin"), QStringLiteral(ATLAS_PDFIUM_PIN));
    result.insert(QStringLiteral("pdfium_version"), QStringLiteral(ATLAS_PDFIUM_VERSION));
    result.insert(QStringLiteral("call_model"), QStringLiteral("serialized-single-thread"));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());
    result.insert(QStringLiteral("query_utf8_sha256"), sha256(query.toUtf8()));
    result.insert(QStringLiteral("query_utf16_length"), query.size());

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
    FPDF_DOCUMENT document = FPDF_LoadMemDocument64(
        pdfBytes.constData(), static_cast<std::size_t>(pdfBytes.size()), nullptr);
    if (document == nullptr) {
        addFailure(failures, QStringLiteral("document-load-failed"));
        result.insert(QStringLiteral("load_error_code"), static_cast<qint64>(FPDF_GetLastError()));
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("passed"), false);
        FPDF_DestroyLibrary();
        const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
        fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
        return 2;
    }

    const std::vector<unsigned short> queryWide = pdfiumWideString(query);
    const int pageCount = FPDF_GetPageCount(document);
    result.insert(QStringLiteral("page_count"), pageCount);

    QElapsedTimer timer;
    timer.start();
    QJsonArray hits;
    int globalOrdinal = 0;

    for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        FPDF_PAGE page = FPDF_LoadPage(document, pageIndex);
        if (page == nullptr) {
            addFailure(failures, QStringLiteral("page-load-failed:%1").arg(pageIndex));
            continue;
        }

        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
        if (textPage == nullptr) {
            addFailure(failures, QStringLiteral("text-page-load-failed:%1").arg(pageIndex));
            FPDF_ClosePage(page);
            continue;
        }

        FPDF_SCHHANDLE search = FPDFText_FindStart(textPage, queryWide.data(), 0, 0);
        if (search == nullptr) {
            addFailure(failures, QStringLiteral("search-start-failed:%1").arg(pageIndex));
            FPDFText_ClosePage(textPage);
            FPDF_ClosePage(page);
            continue;
        }

        int ordinalOnPage = 0;
        while (FPDFText_FindNext(search)) {
            const int start = FPDFText_GetSchResultIndex(search);
            const int count = FPDFText_GetSchCount(search);

            QJsonObject hit;
            hit.insert(QStringLiteral("global_ordinal"), globalOrdinal++);
            hit.insert(QStringLiteral("page"), pageIndex);
            hit.insert(QStringLiteral("ordinal_on_page"), ordinalOnPage++);
            hit.insert(QStringLiteral("engine_char_start"), start);
            hit.insert(QStringLiteral("engine_char_count"), count);

            const int rectCount = FPDFText_CountRects(textPage, start, count);
            hit.insert(QStringLiteral("rect_count"), rectCount);
            if (rectCount > 0) {
                double left = 0.0;
                double top = 0.0;
                double right = 0.0;
                double bottom = 0.0;
                if (FPDFText_GetRect(textPage, 0, &left, &top, &right, &bottom)) {
                    QJsonObject firstRect;
                    firstRect.insert(QStringLiteral("left"), left);
                    firstRect.insert(QStringLiteral("top"), top);
                    firstRect.insert(QStringLiteral("right"), right);
                    firstRect.insert(QStringLiteral("bottom"), bottom);
                    hit.insert(QStringLiteral("first_rect"), firstRect);
                }
            }

            hits.append(hit);
        }

        FPDFText_FindClose(search);
        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);
    }

    result.insert(QStringLiteral("search_ms"), static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0);
    result.insert(QStringLiteral("hit_count"), hits.size());
    result.insert(QStringLiteral("hits"), hits);

    if (parser.isSet(expectCountOption)) {
        const auto expected = parseInt(parser.value(expectCountOption));
        if (!expected.has_value()) {
            addFailure(failures, QStringLiteral("invalid-expect-count"));
        } else if (hits.size() != *expected) {
            addFailure(failures, QStringLiteral("hit-count:%1!=%2").arg(hits.size()).arg(*expected));
        }
    }

    if (parser.isSet(expectFirstPageOption)) {
        const auto expectedPage = parseInt(parser.value(expectFirstPageOption));
        const auto expectedOrdinal = parseInt(parser.value(expectFirstOrdinalOption));
        if (!expectedPage.has_value() || !expectedOrdinal.has_value()) {
            addFailure(failures, QStringLiteral("invalid-first-hit-expectation"));
        } else if (hits.isEmpty()) {
            addFailure(failures, QStringLiteral("expected-first-hit-missing"));
        } else {
            const QJsonObject first = hits.constFirst().toObject();
            if (first.value(QStringLiteral("page")).toInt() != *expectedPage) {
                addFailure(failures, QStringLiteral("first-hit-page-mismatch"));
            }
            if (first.value(QStringLiteral("ordinal_on_page")).toInt() != *expectedOrdinal) {
                addFailure(failures, QStringLiteral("first-hit-ordinal-mismatch"));
            }
        }
    }

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());

    FPDF_CloseDocument(document);
    FPDF_DestroyLibrary();

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
    fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
    return failures.isEmpty() ? 0 : 3;
}
