#include <QColor>
#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QSize>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

namespace {

QJsonObject rgba(const QColor& color)
{
    QJsonObject out;
    out.insert(QStringLiteral("r"), color.red());
    out.insert(QStringLiteral("g"), color.green());
    out.insert(QStringLiteral("b"), color.blue());
    out.insert(QStringLiteral("a"), color.alpha());
    return out;
}

QColor sampleNormalized(const QImage& image, double nx, double ny)
{
    const int x = std::clamp(
        static_cast<int>(std::lround(nx * static_cast<double>(image.width() - 1))),
        0,
        image.width() - 1);
    const int y = std::clamp(
        static_cast<int>(std::lround(ny * static_cast<double>(image.height() - 1))),
        0,
        image.height() - 1);
    return image.pixelColor(x, y);
}

void addSamples(QJsonObject& render, const QImage& image, int pageIndex)
{
    QJsonObject samples;
    if (pageIndex == 0) {
        samples.insert(QStringLiteral("top_left_red"), rgba(sampleNormalized(image, 0.20, 0.1333333333)));
        samples.insert(QStringLiteral("top_right_green"), rgba(sampleNormalized(image, 0.80, 0.1333333333)));
        samples.insert(QStringLiteral("bottom_left_blue"), rgba(sampleNormalized(image, 0.20, 0.8666666667)));
        samples.insert(QStringLiteral("bottom_right_yellow"), rgba(sampleNormalized(image, 0.80, 0.8666666667)));
        samples.insert(QStringLiteral("annotation_probe"), rgba(sampleNormalized(image, 0.50, 0.4233333333)));
    } else if (pageIndex == 1) {
        samples.insert(QStringLiteral("crop_magenta"), rgba(sampleNormalized(image, 0.125, 0.1666666667)));
        samples.insert(QStringLiteral("crop_cyan"), rgba(sampleNormalized(image, 0.875, 0.8333333333)));
        samples.insert(QStringLiteral("crop_top_left_white"), rgba(sampleNormalized(image, 0.03, 0.03)));
        samples.insert(QStringLiteral("crop_bottom_right_white"), rgba(sampleNormalized(image, 0.97, 0.97)));
    } else if (pageIndex == 2) {
        samples.insert(QStringLiteral("rotated_top_left_blue"), rgba(sampleNormalized(image, 0.1333333333, 0.20)));
        samples.insert(QStringLiteral("rotated_top_right_red"), rgba(sampleNormalized(image, 0.8666666667, 0.20)));
        samples.insert(QStringLiteral("rotated_bottom_left_yellow"), rgba(sampleNormalized(image, 0.1333333333, 0.80)));
        samples.insert(QStringLiteral("rotated_bottom_right_green"), rgba(sampleNormalized(image, 0.8666666667, 0.80)));
    }
    render.insert(QStringLiteral("samples"), samples);
}

QJsonObject renderPage(QPdfDocument& document, int pageIndex, int scale, bool annotations)
{
    const QSizeF pointSize = document.pagePointSize(pageIndex);
    const QSize pixelSize(
        std::max(1, static_cast<int>(std::lround(pointSize.width() * scale))),
        std::max(1, static_cast<int>(std::lround(pointSize.height() * scale))));

    QPdfDocumentRenderOptions options;
    if (annotations) {
        options.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);
    }

    const QImage image = document.render(pageIndex, pixelSize, options);

    QJsonObject out;
    out.insert(QStringLiteral("scale"), scale);
    out.insert(QStringLiteral("annotations"), annotations);
    out.insert(QStringLiteral("width"), image.width());
    out.insert(QStringLiteral("height"), image.height());
    out.insert(QStringLiteral("is_null"), image.isNull());
    if (!image.isNull()) {
        addSamples(out, image, pageIndex);
    }
    return out;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        std::fprintf(stderr, "usage: atlas_qt_pdf_fidelity_probe <A011.pdf>\n");
        return 64;
    }

    const QString path = QString::fromLocal8Bit(argv[1]);
    QPdfDocument document;
    const QPdfDocument::Error error = document.load(path);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-fidelity.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("file_name"), QFileInfo(path).fileName());
    result.insert(QStringLiteral("load_error"), static_cast<int>(error));

    QJsonArray failures;
    if (error != QPdfDocument::Error::None) {
        failures.append(QStringLiteral("load-failed"));
    } else if (document.pageCount() != 3) {
        failures.append(QStringLiteral("page-count"));
    }

    QJsonArray pages;
    if (failures.isEmpty()) {
        for (int pageIndex = 0; pageIndex < document.pageCount(); ++pageIndex) {
            const QSizeF pointSize = document.pagePointSize(pageIndex);
            QJsonObject page;
            page.insert(QStringLiteral("index"), pageIndex);
            page.insert(QStringLiteral("visible_width_points"), pointSize.width());
            page.insert(QStringLiteral("visible_height_points"), pointSize.height());

            QJsonArray renders;
            renders.append(renderPage(document, pageIndex, 1, false));
            renders.append(renderPage(document, pageIndex, 2, false));
            if (pageIndex == 0) {
                renders.append(renderPage(document, pageIndex, 1, true));
                renders.append(renderPage(document, pageIndex, 2, true));
            }
            page.insert(QStringLiteral("renders"), renders);
            pages.append(page);
        }
    }

    result.insert(QStringLiteral("pages"), pages);
    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());

    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return failures.isEmpty() ? 0 : 2;
}
