#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <fpdfview.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

namespace {

QJsonObject rgbaAt(FPDF_BITMAP bitmap, double nx, double ny)
{
    const int width = FPDFBitmap_GetWidth(bitmap);
    const int height = FPDFBitmap_GetHeight(bitmap);
    const int stride = FPDFBitmap_GetStride(bitmap);
    const int x = std::clamp(
        static_cast<int>(std::lround(nx * static_cast<double>(width - 1))),
        0,
        width - 1);
    const int y = std::clamp(
        static_cast<int>(std::lround(ny * static_cast<double>(height - 1))),
        0,
        height - 1);

    const auto* base = static_cast<const unsigned char*>(FPDFBitmap_GetBuffer(bitmap));
    const auto* pixel = base + (y * stride) + (x * 4);

    QJsonObject out;
    out.insert(QStringLiteral("r"), static_cast<int>(pixel[2]));
    out.insert(QStringLiteral("g"), static_cast<int>(pixel[1]));
    out.insert(QStringLiteral("b"), static_cast<int>(pixel[0]));
    out.insert(QStringLiteral("a"), static_cast<int>(pixel[3]));
    return out;
}

void addSamples(QJsonObject& render, FPDF_BITMAP bitmap, int pageIndex)
{
    QJsonObject samples;
    if (pageIndex == 0) {
        samples.insert(QStringLiteral("top_left_red"), rgbaAt(bitmap, 0.20, 0.1333333333));
        samples.insert(QStringLiteral("top_right_green"), rgbaAt(bitmap, 0.80, 0.1333333333));
        samples.insert(QStringLiteral("bottom_left_blue"), rgbaAt(bitmap, 0.20, 0.8666666667));
        samples.insert(QStringLiteral("bottom_right_yellow"), rgbaAt(bitmap, 0.80, 0.8666666667));
        samples.insert(QStringLiteral("annotation_probe"), rgbaAt(bitmap, 0.50, 0.4233333333));
    } else if (pageIndex == 1) {
        samples.insert(QStringLiteral("crop_magenta"), rgbaAt(bitmap, 0.125, 0.1666666667));
        samples.insert(QStringLiteral("crop_cyan"), rgbaAt(bitmap, 0.875, 0.8333333333));
        samples.insert(QStringLiteral("crop_top_left_white"), rgbaAt(bitmap, 0.03, 0.03));
        samples.insert(QStringLiteral("crop_bottom_right_white"), rgbaAt(bitmap, 0.97, 0.97));
    } else if (pageIndex == 2) {
        samples.insert(QStringLiteral("rotated_top_left_blue"), rgbaAt(bitmap, 0.1333333333, 0.20));
        samples.insert(QStringLiteral("rotated_top_right_red"), rgbaAt(bitmap, 0.8666666667, 0.20));
        samples.insert(QStringLiteral("rotated_bottom_left_yellow"), rgbaAt(bitmap, 0.1333333333, 0.80));
        samples.insert(QStringLiteral("rotated_bottom_right_green"), rgbaAt(bitmap, 0.8666666667, 0.80));
    }
    render.insert(QStringLiteral("samples"), samples);
}

QJsonObject renderPage(FPDF_PAGE page, int pageIndex, int scale, bool annotations)
{
    const double widthPoints = FPDF_GetPageWidthF(page);
    const double heightPoints = FPDF_GetPageHeightF(page);
    const int width = std::max(1, static_cast<int>(std::lround(widthPoints * scale)));
    const int height = std::max(1, static_cast<int>(std::lround(heightPoints * scale)));

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    QJsonObject out;
    out.insert(QStringLiteral("scale"), scale);
    out.insert(QStringLiteral("annotations"), annotations);
    out.insert(QStringLiteral("width"), width);
    out.insert(QStringLiteral("height"), height);

    if (bitmap == nullptr) {
        out.insert(QStringLiteral("allocation_failed"), true);
        return out;
    }

    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFFU);
    const int flags = annotations ? FPDF_ANNOT : 0;
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, flags);
    addSamples(out, bitmap, pageIndex);
    FPDFBitmap_Destroy(bitmap);
    return out;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        std::fprintf(stderr, "usage: atlas_pdfium_fidelity_probe <A011.pdf>\n");
        return 64;
    }

    const QString path = QString::fromLocal8Bit(argv[1]);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return 2;
    }
    const QByteArray bytes = file.readAll();
    file.close();

    FPDF_InitLibrary();
    FPDF_DOCUMENT document = FPDF_LoadMemDocument64(
        bytes.constData(), static_cast<std::size_t>(bytes.size()), nullptr);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-fidelity.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("file_name"), QFileInfo(path).fileName());

    QJsonArray failures;
    if (document == nullptr) {
        failures.append(QStringLiteral("load-failed"));
    } else if (FPDF_GetPageCount(document) != 3) {
        failures.append(QStringLiteral("page-count"));
    }

    QJsonArray pages;
    if (document != nullptr && failures.isEmpty()) {
        for (int pageIndex = 0; pageIndex < FPDF_GetPageCount(document); ++pageIndex) {
            FPDF_PAGE pageHandle = FPDF_LoadPage(document, pageIndex);
            if (pageHandle == nullptr) {
                failures.append(QStringLiteral("page-load-failed:%1").arg(pageIndex));
                continue;
            }

            QJsonObject page;
            page.insert(QStringLiteral("index"), pageIndex);
            page.insert(QStringLiteral("visible_width_points"), FPDF_GetPageWidthF(pageHandle));
            page.insert(QStringLiteral("visible_height_points"), FPDF_GetPageHeightF(pageHandle));
            page.insert(QStringLiteral("rotation_quarters"), FPDFPage_GetRotation(pageHandle));

            QJsonArray renders;
            renders.append(renderPage(pageHandle, pageIndex, 1, false));
            renders.append(renderPage(pageHandle, pageIndex, 2, false));
            if (pageIndex == 0) {
                renders.append(renderPage(pageHandle, pageIndex, 1, true));
                renders.append(renderPage(pageHandle, pageIndex, 2, true));
            }
            page.insert(QStringLiteral("renders"), renders);
            pages.append(page);
            FPDF_ClosePage(pageHandle);
        }
    }

    if (document != nullptr) {
        FPDF_CloseDocument(document);
    }
    FPDF_DestroyLibrary();

    result.insert(QStringLiteral("pages"), pages);
    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());

    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return failures.isEmpty() ? 0 : 2;
}
