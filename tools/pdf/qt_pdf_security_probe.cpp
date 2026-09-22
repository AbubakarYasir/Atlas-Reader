#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfDocument>
#include <QStringList>

#include <cstddef>
#include <cstdio>
#include <optional>

namespace {

QString errorName(QPdfDocument::Error error)
{
    switch (error) {
    case QPdfDocument::Error::None:
        return QStringLiteral("none");
    case QPdfDocument::Error::Unknown:
        return QStringLiteral("unknown");
    case QPdfDocument::Error::DataNotYetAvailable:
        return QStringLiteral("data-not-yet-available");
    case QPdfDocument::Error::FileNotFound:
        return QStringLiteral("file-not-found");
    case QPdfDocument::Error::InvalidFileFormat:
        return QStringLiteral("invalid-file-format");
    case QPdfDocument::Error::IncorrectPassword:
        return QStringLiteral("incorrect-password");
    case QPdfDocument::Error::UnsupportedSecurityScheme:
        return QStringLiteral("unsupported-security-scheme");
    }
    return QStringLiteral("unmapped");
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

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_qt_pdf_security_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Atlas N2 focused Qt PDF security/open qualification probe"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption fixtureIdOption(
        QStringLiteral("fixture-id"),
        QStringLiteral("Sanitized fixture identifier recorded in probe output."),
        QStringLiteral("id"));
    const QCommandLineOption passwordOption(
        QStringLiteral("password"),
        QStringLiteral("Password supplied to QPdfDocument; never emitted in probe output."),
        QStringLiteral("password"));
    const QCommandLineOption expectLoadErrorOption(
        QStringLiteral("expect-load-error"),
        QStringLiteral("Expected load result name: none, invalid-file-format, incorrect-password, unsupported-security-scheme, etc."),
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
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-security-probe.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());
    result.insert(QStringLiteral("password_supplied"), parser.isSet(passwordOption));
    result.insert(QStringLiteral("expected_load_error"), expectedLoadError);

    QJsonArray failures;
    QPdfDocument document;
    if (parser.isSet(passwordOption)) {
        document.setPassword(parser.value(passwordOption));
    }

    QElapsedTimer timer;
    timer.start();
    const QPdfDocument::Error loadError = document.load(pdfPath);
    const QString actualLoadError = errorName(loadError);
    result.insert(QStringLiteral("open_ms"), static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0);
    result.insert(QStringLiteral("load_error"), actualLoadError);
    result.insert(QStringLiteral("load_error_code"), static_cast<int>(loadError));
    result.insert(QStringLiteral("status_code"), static_cast<int>(document.status()));

    if (actualLoadError != expectedLoadError) {
        addFailure(
            failures,
            QStringLiteral("load-error:%1!=%2").arg(actualLoadError, expectedLoadError));
    }

    if (loadError == QPdfDocument::Error::None) {
        const int pageCount = document.pageCount();
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
                labels.append(document.pageLabel(page));
            }
            result.insert(QStringLiteral("page_labels"), QJsonArray::fromStringList(labels));
            const QStringList expected = parser.value(expectLabelsOption).split(QLatin1Char(','), Qt::KeepEmptyParts);
            if (labels != expected) {
                addFailure(failures, QStringLiteral("page-labels-mismatch"));
            }
        }
    } else if (parser.isSet(expectPagesOption) || parser.isSet(expectLabelsOption)) {
        addFailure(failures, QStringLiteral("successful-document-assertions-requested-after-load-failure"));
    }

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
    fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
    return failures.isEmpty() ? 0 : 3;
}
