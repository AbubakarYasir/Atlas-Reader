#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <fpdf_text.h>
#include <fpdfview.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace {

std::vector<unsigned short> wide(const QString& value)
{
    std::vector<unsigned short> out(static_cast<std::size_t>(value.size()) + 1U, 0);
    for (qsizetype i = 0; i < value.size(); ++i) {
        out[static_cast<std::size_t>(i)] = value.at(i).unicode();
    }
    return out;
}

QJsonObject renderOne(FPDF_PAGE page, const QString& fixtureId, int scale, const QString& outputDir, QJsonArray& failures)
{
    const double widthPoints = FPDF_GetPageWidthF(page);
    const double heightPoints = FPDF_GetPageHeightF(page);
    const int width = (std::max)(1, static_cast<int>(std::lround(widthPoints * scale)));
    const int height = (std::max)(1, static_cast<int>(std::lround(heightPoints * scale)));

    QJsonObject result;
    result.insert(QStringLiteral("scale"), scale);
    result.insert(QStringLiteral("width"), width);
    result.insert(QStringLiteral("height"), height);

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 1);
    if (bitmap == nullptr) {
        failures.append(QStringLiteral("bitmap-allocation:%1x").arg(scale));
        return result;
    }

    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFFU);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);

    const int stride = FPDFBitmap_GetStride(bitmap);
    const auto* buffer = static_cast<const uchar*>(FPDFBitmap_GetBuffer(bitmap));
    QImage wrapped(buffer, width, height, stride, QImage::Format_ARGB32);
    const QImage copy = wrapped.copy();
    const QString png = QDir(outputDir).filePath(
        QStringLiteral("pdfium-%1-%2x.png").arg(fixtureId).arg(scale));
    result.insert(QStringLiteral("png"), png);
    if (!copy.save(png, "PNG")) {
        failures.append(QStringLiteral("png-save-failed:%1x").arg(scale));
    }

    FPDFBitmap_Destroy(bitmap);
    return result;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        std::fprintf(stderr, "usage: atlas_pdfium_content_probe <fixture-id> <pdf> <output-dir>\n");
        return 64;
    }

    const QString fixtureId = QString::fromLocal8Bit(argv[1]);
    const QString pdfPath = QString::fromLocal8Bit(argv[2]);
    const QString outputDir = QString::fromLocal8Bit(argv[3]);
    QDir().mkpath(outputDir);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-content-fidelity.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());

    QJsonArray failures;
    QFile file(pdfPath);
    if (!file.open(QIODevice::ReadOnly)) {
        failures.append(QStringLiteral("fixture-open-failed"));
    }
    const QByteArray bytes = file.isOpen() ? file.readAll() : QByteArray();
    file.close();

    FPDF_InitLibrary();
    FPDF_DOCUMENT document = failures.isEmpty()
        ? FPDF_LoadMemDocument64(bytes.constData(), static_cast<std::size_t>(bytes.size()), nullptr)
        : nullptr;
    if (document == nullptr) {
        failures.append(QStringLiteral("load-failed"));
    }

    if (document != nullptr) {
        const int pageCount = FPDF_GetPageCount(document);
        result.insert(QStringLiteral("page_count"), pageCount);
        if (pageCount != 1) {
            failures.append(QStringLiteral("page-count"));
        } else {
            FPDF_PAGE page = FPDF_LoadPage(document, 0);
            if (page == nullptr) {
                failures.append(QStringLiteral("page-load-failed"));
            } else {
                result.insert(QStringLiteral("width_points"), FPDF_GetPageWidthF(page));
                result.insert(QStringLiteral("height_points"), FPDF_GetPageHeightF(page));

                FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
                if (textPage == nullptr) {
                    failures.append(QStringLiteral("text-page-load-failed"));
                } else {
                    const int count = FPDFText_CountChars(textPage);
                    result.insert(QStringLiteral("text_char_count"), count);
                    const auto query = wide(QStringLiteral("Atlas"));
                    FPDF_SCHHANDLE search = FPDFText_FindStart(textPage, query.data(), 0, 0);
                    int hits = 0;
                    if (search != nullptr) {
                        while (FPDFText_FindNext(search)) {
                            ++hits;
                        }
                        FPDFText_FindClose(search);
                    }
                    result.insert(QStringLiteral("search_hit_count"), hits);
                    FPDFText_ClosePage(textPage);
                }

                QJsonArray renders;
                for (int scale : {1, 2}) {
                    renders.append(renderOne(page, fixtureId, scale, outputDir, failures));
                }
                result.insert(QStringLiteral("renders"), renders);
                FPDF_ClosePage(page);
            }
        }
        FPDF_CloseDocument(document);
    }
    FPDF_DestroyLibrary();

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());
    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return failures.isEmpty() ? 0 : 2;
}
