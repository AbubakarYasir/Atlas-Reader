#include <QByteArray>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <fpdf_doc.h>
#include <fpdfview.h>

#include <cstddef>
#include <cstdio>
#include <optional>

namespace {

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

QString pageLabel(FPDF_DOCUMENT document, int pageIndex)
{
    const unsigned long bytesRequired = FPDF_GetPageLabel(document, pageIndex, nullptr, 0);
    if (bytesRequired < 2) {
        return QString::number(pageIndex + 1);
    }

    QByteArray buffer(static_cast<qsizetype>(bytesRequired), '\0');
    const unsigned long written = FPDF_GetPageLabel(
        document, pageIndex, buffer.data(), static_cast<unsigned long>(buffer.size()));
    if (written < 2 || written > static_cast<unsigned long>(buffer.size())) {
        return {};
    }

    const auto* utf16 = reinterpret_cast<const char16_t*>(buffer.constData());
    return QString::fromUtf16(utf16, static_cast<qsizetype>((written / 2) - 1));
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_pdfium_security_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Atlas N2 focused PDFium security/open qualification probe"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption fixtureIdOption(
        QStringLiteral("fixture-id"),
        QStringLiteral("Sanitized fixture identifier recorded in probe output."),
        QStringLiteral("id"));
    const QCommandLineOption passwordOption(
        QStringLiteral("password"),
        QStringLiteral("Password supplied to PDFium; never emitted in probe output."),
        QStringLiteral("password"));
    const QCommandLineOption expectLoadErrorOption(
        QStringLiteral("expect-load-error"),
        QStringLiteral("Expected load result name: success, format, password, security, etc."),
        QStringLiteral("name"));
    const QCommandLineOption expectPagesOption(
        QStringLiteral("expect-pages"),
        QStringLiteral("Expected page count after successful load."),
        QStringLiteral("count"));
    const QCommandLineOption expectLabelsOption(
        QStringLiteral("expect-labels"),
        QStringLiteral("Comma-separated expected display page labels after successful load."),
        QStringLiteral("labels"));

    parser.addOption(fixtureIdOption);
    parser.addOption(passwordOption);
    parser.addOption(expectLoadErrorOption);
    parser.addOption(expectPagesOption);
    parser.addOption(expectLabelsOption);
    parser.addPositionalArgument(QStringLiteral("pdf"), QStringLiteral("PDF fixture path."));
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.size() != 1 || !parser.isSet(expectLoadErrorOption)) {
        parser.showHelp(64);
    }

    const QString pdfPath = positional.constFirst();
    const QString fixtureId = parser.value(fixtureIdOption).isEmpty()
        ? QFileInfo(pdfPath).completeBaseName()
        : parser.value(fixtureIdOption);
    const QString expectedLoadError = parser.value(expectLoadErrorOption);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-security-probe.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("pdfium_pin"), QStringLiteral(ATLAS_PDFIUM_PIN));
    result.insert(QStringLiteral("pdfium_version"), QStringLiteral(ATLAS_PDFIUM_VERSION));
    result.insert(QStringLiteral("call_model"), QStringLiteral("serialized-single-thread"));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());
    result.insert(QStringLiteral("password_supplied"), parser.isSet(passwordOption));
    result.insert(QStringLiteral("expected_load_error"), expectedLoadError);

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
    const QByteArray bytes = file.readAll();
    file.close();

    QByteArray passwordBytes;
    const char* password = nullptr;
    if (parser.isSet(passwordOption)) {
        passwordBytes = parser.value(passwordOption).toUtf8();
        password = passwordBytes.constData();
    }

    FPDF_InitLibrary();
    QElapsedTimer timer;
    timer.start();
    FPDF_DOCUMENT document = FPDF_LoadMemDocument64(
        bytes.constData(), static_cast<std::size_t>(bytes.size()), password);
    const double openMs = static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0;
    const unsigned long loadErrorCode = document == nullptr ? FPDF_GetLastError() : FPDF_ERR_SUCCESS;
    const QString actualLoadError = pdfiumErrorName(loadErrorCode);

    result.insert(QStringLiteral("open_ms"), openMs);
    result.insert(QStringLiteral("load_error"), actualLoadError);
    result.insert(QStringLiteral("load_error_code"), static_cast<qint64>(loadErrorCode));

    if (actualLoadError != expectedLoadError) {
        addFailure(
            failures,
            QStringLiteral("load-error:%1!=%2").arg(actualLoadError, expectedLoadError));
    }

    if (document != nullptr) {
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

        if (parser.isSet(expectLabelsOption)) {
            QStringList labels;
            for (int page = 0; page < pageCount; ++page) {
                labels.append(pageLabel(document, page));
            }
            result.insert(QStringLiteral("page_labels"), QJsonArray::fromStringList(labels));
            const QStringList expected = parser.value(expectLabelsOption).split(QLatin1Char(','), Qt::KeepEmptyParts);
            if (labels != expected) {
                addFailure(failures, QStringLiteral("page-labels-mismatch"));
            }
        }

        FPDF_CloseDocument(document);
    } else if (parser.isSet(expectPagesOption) || parser.isSet(expectLabelsOption)) {
        addFailure(failures, QStringLiteral("successful-document-assertions-requested-after-load-failure"));
    }

    FPDF_DestroyLibrary();

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
    fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
    return failures.isEmpty() ? 0 : 3;
}
