#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QModelIndex>
#include <QPdfDocument>
#include <QPdfSearchModel>
#include <QPointF>
#include <QThread>

#include <cstddef>
#include <cstdio>
#include <optional>

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

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_qt_pdf_search_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Atlas N2 focused Qt PDF search qualification probe"));
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
    const QCommandLineOption timeoutOption(
        QStringLiteral("timeout-ms"),
        QStringLiteral("Maximum time to allow the Qt search model to settle."),
        QStringLiteral("milliseconds"),
        QStringLiteral("5000"));

    parser.addOption(fixtureIdOption);
    parser.addOption(queryOption);
    parser.addOption(expectCountOption);
    parser.addOption(expectFirstPageOption);
    parser.addOption(expectFirstOrdinalOption);
    parser.addOption(timeoutOption);
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
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-search-probe.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());
    result.insert(QStringLiteral("query_utf8_sha256"), sha256(query.toUtf8()));
    result.insert(QStringLiteral("query_utf16_length"), query.size());

    QJsonArray failures;
    QPdfDocument document;
    const QPdfDocument::Error loadError = document.load(pdfPath);
    result.insert(QStringLiteral("load_error"), errorName(loadError));
    result.insert(QStringLiteral("load_error_code"), static_cast<int>(loadError));

    if (loadError != QPdfDocument::Error::None) {
        addFailure(failures, QStringLiteral("document-load-failed:%1").arg(errorName(loadError)));
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("passed"), false);
        const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
        fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
        return 2;
    }

    const auto timeoutMs = parseInt(parser.value(timeoutOption));
    if (!timeoutMs.has_value() || *timeoutMs <= 0) {
        addFailure(failures, QStringLiteral("invalid-timeout-ms"));
    }

    QPdfSearchModel model;
    model.setDocument(&document);

    QElapsedTimer timer;
    timer.start();
    model.setSearchString(query);

    const int effectiveTimeoutMs = timeoutMs.has_value() && *timeoutMs > 0 ? *timeoutMs : 5000;
    constexpr qint64 minimumObservationMs = 500;
    constexpr qint64 stableWindowMs = 150;
    int lastCount = model.rowCount();
    qint64 lastChangeMs = timer.elapsed();
    bool settled = false;

    while (timer.elapsed() < effectiveTimeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        const int currentCount = model.rowCount();
        if (currentCount != lastCount) {
            lastCount = currentCount;
            lastChangeMs = timer.elapsed();
        }
        if (timer.elapsed() >= minimumObservationMs
            && timer.elapsed() - lastChangeMs >= stableWindowMs) {
            settled = true;
            break;
        }
        QThread::msleep(5);
    }

    result.insert(QStringLiteral("search_ms"), static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0);
    result.insert(QStringLiteral("settled"), settled);
    if (!settled) {
        addFailure(failures, QStringLiteral("search-timeout"));
    }

    const int pageRole = static_cast<int>(QPdfSearchModel::Role::Page);
    const int indexOnPageRole = static_cast<int>(QPdfSearchModel::Role::IndexOnPage);
    const int locationRole = static_cast<int>(QPdfSearchModel::Role::Location);
    const int contextBeforeRole = static_cast<int>(QPdfSearchModel::Role::ContextBefore);
    const int contextAfterRole = static_cast<int>(QPdfSearchModel::Role::ContextAfter);

    QJsonArray hits;
    QHash<int, int> nextOrdinalOnPage;
    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0, QModelIndex());
        const int page = model.data(index, pageRole).toInt();
        const int normalizedOrdinal = nextOrdinalOnPage.value(page, 0);
        nextOrdinalOnPage.insert(page, normalizedOrdinal + 1);
        const QPointF location = model.data(index, locationRole).toPointF();

        QJsonObject hit;
        hit.insert(QStringLiteral("global_ordinal"), row);
        hit.insert(QStringLiteral("page"), page);
        hit.insert(QStringLiteral("ordinal_on_page"), normalizedOrdinal);
        hit.insert(QStringLiteral("engine_index_on_page"), model.data(index, indexOnPageRole).toInt());
        hit.insert(QStringLiteral("location_x"), location.x());
        hit.insert(QStringLiteral("location_y"), location.y());
        hit.insert(QStringLiteral("context_before_utf8_sha256"), sha256(model.data(index, contextBeforeRole).toString().toUtf8()));
        hit.insert(QStringLiteral("context_after_utf8_sha256"), sha256(model.data(index, contextAfterRole).toString().toUtf8()));
        hits.append(hit);
    }

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

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
    fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
    return failures.isEmpty() ? 0 : 3;
}
