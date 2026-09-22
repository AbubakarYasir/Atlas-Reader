#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QStringList>

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

QString sha256(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QByteArray imageBytes(const QImage& image)
{
    if (image.isNull() || image.constBits() == nullptr || image.sizeInBytes() <= 0) {
        return {};
    }

    return QByteArray(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes());
}

std::optional<int> parseInt(const QString& value)
{
    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (!ok) {
        return std::nullopt;
    }
    return parsed;
}

void addFailure(QJsonArray& failures, const QString& message)
{
    failures.append(message);
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("atlas_qt_pdf_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Atlas N2 focused Qt PDF qualification probe"));
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
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-probe.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("atlas_version"), QStringLiteral(ATLAS_VERSION_STRING));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());

    QJsonArray failures;
    QPdfDocument document;

    QElapsedTimer openTimer;
    openTimer.start();
    const QPdfDocument::Error loadError = document.load(pdfPath);
    const qint64 openElapsedNs = openTimer.nsecsElapsed();

    result.insert(QStringLiteral("open_ms"), static_cast<double>(openElapsedNs) / 1'000'000.0);
    result.insert(QStringLiteral("load_error"), errorName(loadError));
    result.insert(QStringLiteral("load_error_code"), static_cast<int>(loadError));
    result.insert(QStringLiteral("status_code"), static_cast<int>(document.status()));

    if (loadError != QPdfDocument::Error::None) {
        addFailure(failures, QStringLiteral("document-load-failed:%1").arg(errorName(loadError)));
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("passed"), false);
        const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
        fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);
        return 2;
    }

    const int pageCount = document.pageCount();
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
        QJsonObject page;
        page.insert(QStringLiteral("index"), pageIndex);

        const QString label = document.pageLabel(pageIndex);
        labels.append(label);
        page.insert(QStringLiteral("label"), label);

        const QSizeF sizePoints = document.pagePointSize(pageIndex);
        page.insert(QStringLiteral("width_points"), sizePoints.width());
        page.insert(QStringLiteral("height_points"), sizePoints.height());

        QElapsedTimer textTimer;
        textTimer.start();
        const QPdfSelection selection = document.getAllText(pageIndex);
        const qint64 textElapsedNs = textTimer.nsecsElapsed();
        const QString text = selection.text();
        extractedTexts.append(text);

        page.insert(QStringLiteral("text_ms"), static_cast<double>(textElapsedNs) / 1'000'000.0);
        page.insert(QStringLiteral("text_utf16_length"), text.size());
        page.insert(QStringLiteral("text_utf8_sha256"), sha256(text.toUtf8()));

        pages.append(page);
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
            QElapsedTimer renderTimer;
            renderTimer.start();
            const QImage image = document.render(*pageIndex, QSize(*width, *height));
            const qint64 renderElapsedNs = renderTimer.nsecsElapsed();

            QJsonObject render;
            render.insert(QStringLiteral("page_index"), *pageIndex);
            render.insert(QStringLiteral("requested_width"), *width);
            render.insert(QStringLiteral("requested_height"), *height);
            render.insert(QStringLiteral("actual_width"), image.width());
            render.insert(QStringLiteral("actual_height"), image.height());
            render.insert(
                QStringLiteral("render_ms"), static_cast<double>(renderElapsedNs) / 1'000'000.0);
            render.insert(QStringLiteral("format_code"), static_cast<int>(image.format()));
            render.insert(QStringLiteral("pixel_sha256"), sha256(imageBytes(image)));
            render.insert(QStringLiteral("is_null"), image.isNull());
            result.insert(QStringLiteral("render"), render);

            if (image.isNull()) {
                addFailure(failures, QStringLiteral("render-returned-null-image"));
            }
        }
    }

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());

    const QByteArray output = QJsonDocument(result).toJson(QJsonDocument::Indented);
    fwrite(output.constData(), 1, static_cast<std::size_t>(output.size()), stdout);

    return failures.isEmpty() ? 0 : 3;
}
