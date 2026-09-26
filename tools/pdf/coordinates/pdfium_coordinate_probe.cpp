#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <fpdf_doc.h>
#include <fpdf_text.h>
#include <fpdfview.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace {

std::vector<unsigned short> pdfiumWideString(const QString& value)
{
    std::vector<unsigned short> result(static_cast<std::size_t>(value.size()) + 1U, 0);
    for (qsizetype i = 0; i < value.size(); ++i) {
        result[static_cast<std::size_t>(i)] = value.at(i).unicode();
    }
    return result;
}

QString actionUri(FPDF_DOCUMENT document, FPDF_ACTION action)
{
    if (action == nullptr || FPDFAction_GetType(action) != PDFACTION_URI) {
        return {};
    }
    const unsigned long required = FPDFAction_GetURIPath(document, action, nullptr, 0);
    if (required < 1) {
        return {};
    }
    QByteArray buffer(static_cast<qsizetype>(required), '\0');
    const unsigned long written = FPDFAction_GetURIPath(
        document, action, buffer.data(), static_cast<unsigned long>(buffer.size()));
    if (written < 1 || written > static_cast<unsigned long>(buffer.size())) {
        return {};
    }
    if (!buffer.isEmpty() && buffer.back() == '\0') {
        buffer.chop(1);
    }
    return QString::fromUtf8(buffer);
}

FPDF_DEST actionDestination(FPDF_DOCUMENT document, FPDF_ACTION action)
{
    if (action == nullptr || FPDFAction_GetType(action) != PDFACTION_GOTO) {
        return nullptr;
    }
    return FPDFAction_GetDest(document, action);
}

QJsonObject rawRectJson(double left, double top, double right, double bottom)
{
    QJsonObject out;
    out.insert(QStringLiteral("left"), left);
    out.insert(QStringLiteral("top"), top);
    out.insert(QStringLiteral("right"), right);
    out.insert(QStringLiteral("bottom"), bottom);
    return out;
}

bool pagePointToAtlas(FPDF_PAGE page, double x, double y, double* outX, double* outY)
{
    constexpr int precision = 100;
    const int width = (std::max)(1, static_cast<int>(std::lround(FPDF_GetPageWidthF(page) * precision)));
    const int height = (std::max)(1, static_cast<int>(std::lround(FPDF_GetPageHeightF(page) * precision)));
    int deviceX = 0;
    int deviceY = 0;
    if (!FPDF_PageToDevice(page, 0, 0, width, height, 0, x, y, &deviceX, &deviceY)) {
        return false;
    }
    *outX = static_cast<double>(deviceX) / precision;
    *outY = static_cast<double>(deviceY) / precision;
    return true;
}

QJsonObject atlasRect(FPDF_PAGE page, double left, double top, double right, double bottom, bool* ok)
{
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    const bool first = pagePointToAtlas(page, left, top, &x1, &y1);
    const bool second = pagePointToAtlas(page, right, bottom, &x2, &y2);
    *ok = first && second;

    QJsonObject out;
    if (!*ok) {
        return out;
    }
    const double minX = (std::min)(x1, x2);
    const double minY = (std::min)(y1, y2);
    const double maxX = (std::max)(x1, x2);
    const double maxY = (std::max)(y1, y2);
    out.insert(QStringLiteral("x"), minX);
    out.insert(QStringLiteral("y"), minY);
    out.insert(QStringLiteral("width"), maxX - minX);
    out.insert(QStringLiteral("height"), maxY - minY);
    return out;
}

FPDF_DOCUMENT loadDocument(const QString& path, QByteArray* bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    *bytes = file.readAll();
    file.close();
    return FPDF_LoadMemDocument64(bytes->constData(), static_cast<std::size_t>(bytes->size()), nullptr);
}

QJsonArray collectA003Links(FPDF_DOCUMENT document, QJsonArray& failures)
{
    QJsonArray links;
    FPDF_PAGE page = FPDF_LoadPage(document, 0);
    if (page == nullptr) {
        failures.append(QStringLiteral("a003-page-load-failed"));
        return links;
    }

    int position = 0;
    FPDF_LINK link = nullptr;
    int row = 0;
    while (FPDFLink_Enumerate(page, &position, &link)) {
        FS_RECTF rect{};
        if (!FPDFLink_GetAnnotRect(link, &rect)) {
            failures.append(QStringLiteral("a003-link-rect-missing"));
            continue;
        }
        bool converted = false;
        const QJsonObject normalized = atlasRect(page, rect.left, rect.top, rect.right, rect.bottom, &converted);
        if (!converted) {
            failures.append(QStringLiteral("a003-link-conversion-failed"));
        }

        FPDF_DEST dest = FPDFLink_GetDest(document, link);
        FPDF_ACTION action = FPDFLink_GetAction(link);
        if (dest == nullptr) {
            dest = actionDestination(document, action);
        }

        QJsonObject item;
        item.insert(QStringLiteral("row"), row++);
        item.insert(QStringLiteral("source_page"), 0);
        item.insert(QStringLiteral("destination_page"), dest != nullptr ? FPDFDest_GetDestPageIndex(document, dest) : -1);
        item.insert(QStringLiteral("url"), actionUri(document, action));
        item.insert(QStringLiteral("raw_pdfium_rect"), rawRectJson(rect.left, rect.top, rect.right, rect.bottom));
        item.insert(QStringLiteral("atlas_rect"), normalized);
        links.append(item);
    }

    FPDF_ClosePage(page);
    if (links.size() != 2) {
        failures.append(QStringLiteral("a003-link-count:%1").arg(links.size()));
    }
    return links;
}

QJsonArray collectA012Destinations(FPDF_DOCUMENT document, QJsonArray& failures)
{
    QJsonArray destinations;
    FPDF_PAGE sourcePage = FPDF_LoadPage(document, 0);
    if (sourcePage == nullptr) {
        failures.append(QStringLiteral("a012-source-page-load-failed"));
        return destinations;
    }

    int position = 0;
    FPDF_LINK link = nullptr;
    int row = 0;
    while (FPDFLink_Enumerate(sourcePage, &position, &link)) {
        FPDF_DEST dest = FPDFLink_GetDest(document, link);
        if (dest == nullptr) {
            continue;
        }
        const int destinationPage = FPDFDest_GetDestPageIndex(document, dest);
        FPDF_BOOL hasX = false;
        FPDF_BOOL hasY = false;
        FPDF_BOOL hasZoom = false;
        FS_FLOAT x = 0;
        FS_FLOAT y = 0;
        FS_FLOAT zoom = 0;
        if (!FPDFDest_GetLocationInPage(dest, &hasX, &hasY, &hasZoom, &x, &y, &zoom)) {
            failures.append(QStringLiteral("a012-destination-location-read-failed:%1").arg(row));
            continue;
        }

        FPDF_PAGE targetPage = FPDF_LoadPage(document, destinationPage);
        if (targetPage == nullptr) {
            failures.append(QStringLiteral("a012-target-page-load-failed:%1").arg(destinationPage));
            continue;
        }

        double atlasX = 0.0;
        double atlasY = 0.0;
        const bool converted = hasX && hasY && pagePointToAtlas(targetPage, x, y, &atlasX, &atlasY);
        if (!converted) {
            failures.append(QStringLiteral("a012-destination-conversion-failed:%1").arg(row));
        }

        QJsonObject atlasPoint;
        atlasPoint.insert(QStringLiteral("x"), atlasX);
        atlasPoint.insert(QStringLiteral("y"), atlasY);

        QJsonObject item;
        item.insert(QStringLiteral("row"), row++);
        item.insert(QStringLiteral("source_page"), 0);
        item.insert(QStringLiteral("destination_page"), destinationPage);
        item.insert(QStringLiteral("raw_pdfium_x"), static_cast<double>(x));
        item.insert(QStringLiteral("raw_pdfium_y"), static_cast<double>(y));
        item.insert(QStringLiteral("has_x"), static_cast<bool>(hasX));
        item.insert(QStringLiteral("has_y"), static_cast<bool>(hasY));
        item.insert(QStringLiteral("has_zoom"), static_cast<bool>(hasZoom));
        item.insert(QStringLiteral("zoom"), static_cast<double>(zoom));
        item.insert(QStringLiteral("atlas_destination"), atlasPoint);
        destinations.append(item);
        FPDF_ClosePage(targetPage);
    }

    FPDF_ClosePage(sourcePage);
    if (destinations.size() != 2) {
        failures.append(QStringLiteral("a012-destination-count:%1").arg(destinations.size()));
    }
    return destinations;
}

QJsonObject searchOne(FPDF_DOCUMENT document, int pageIndex, const QString& query, QJsonArray& failures)
{
    QJsonObject out;
    out.insert(QStringLiteral("page"), pageIndex);
    out.insert(QStringLiteral("query"), query);

    FPDF_PAGE page = FPDF_LoadPage(document, pageIndex);
    if (page == nullptr) {
        failures.append(QStringLiteral("a011-page-load-failed:%1").arg(pageIndex));
        return out;
    }
    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    if (textPage == nullptr) {
        failures.append(QStringLiteral("a011-text-page-load-failed:%1").arg(pageIndex));
        FPDF_ClosePage(page);
        return out;
    }

    const std::vector<unsigned short> wide = pdfiumWideString(query);
    FPDF_SCHHANDLE search = FPDFText_FindStart(textPage, wide.data(), 0, 0);
    if (search == nullptr || !FPDFText_FindNext(search)) {
        failures.append(QStringLiteral("a011-search-hit-missing:%1").arg(pageIndex));
        if (search != nullptr) {
            FPDFText_FindClose(search);
        }
        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);
        return out;
    }

    const int start = FPDFText_GetSchResultIndex(search);
    const int count = FPDFText_GetSchCount(search);
    const int rectCount = FPDFText_CountRects(textPage, start, count);
    QJsonArray rawRects;
    QJsonArray normalizedRects;
    for (int i = 0; i < rectCount; ++i) {
        double left = 0.0;
        double top = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        if (!FPDFText_GetRect(textPage, i, &left, &top, &right, &bottom)) {
            failures.append(QStringLiteral("a011-search-rect-read-failed:%1:%2").arg(pageIndex).arg(i));
            continue;
        }
        rawRects.append(rawRectJson(left, top, right, bottom));
        bool converted = false;
        normalizedRects.append(atlasRect(page, left, top, right, bottom, &converted));
        if (!converted) {
            failures.append(QStringLiteral("a011-search-rect-conversion-failed:%1:%2").arg(pageIndex).arg(i));
        }
    }

    out.insert(QStringLiteral("visible_width_points"), FPDF_GetPageWidthF(page));
    out.insert(QStringLiteral("visible_height_points"), FPDF_GetPageHeightF(page));
    out.insert(QStringLiteral("engine_char_start"), start);
    out.insert(QStringLiteral("engine_char_count"), count);
    out.insert(QStringLiteral("raw_pdfium_rects"), rawRects);
    out.insert(QStringLiteral("atlas_rects"), normalizedRects);

    FPDFText_FindClose(search);
    FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);

    if (rectCount < 1) {
        failures.append(QStringLiteral("a011-search-rectangles-empty:%1").arg(pageIndex));
    }
    return out;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        std::fprintf(stderr, "usage: atlas_pdfium_coordinate_probe <A003.pdf> <A011.pdf> <A012.pdf>\n");
        return 64;
    }

    const QString a003Path = QString::fromLocal8Bit(argv[1]);
    const QString a011Path = QString::fromLocal8Bit(argv[2]);
    const QString a012Path = QString::fromLocal8Bit(argv[3]);
    QJsonArray failures;

    FPDF_InitLibrary();
    QByteArray a003Bytes;
    QByteArray a011Bytes;
    QByteArray a012Bytes;
    FPDF_DOCUMENT a003 = loadDocument(a003Path, &a003Bytes);
    FPDF_DOCUMENT a011 = loadDocument(a011Path, &a011Bytes);
    FPDF_DOCUMENT a012 = loadDocument(a012Path, &a012Bytes);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.pdfium-coordinate.v2"));
    result.insert(QStringLiteral("engine"), QStringLiteral("pdfium"));
    result.insert(QStringLiteral("pdfium_pin"), QStringLiteral(ATLAS_PDFIUM_PIN));
    result.insert(QStringLiteral("pdfium_version"), QStringLiteral(ATLAS_PDFIUM_VERSION));
    result.insert(QStringLiteral("call_model"), QStringLiteral("serialized-single-thread"));
    result.insert(QStringLiteral("atlas_page_space"), QStringLiteral("effective-visible-page; origin=top-left; x-right; y-down; units=points"));
    result.insert(QStringLiteral("a003_file"), QFileInfo(a003Path).fileName());
    result.insert(QStringLiteral("a011_file"), QFileInfo(a011Path).fileName());
    result.insert(QStringLiteral("a012_file"), QFileInfo(a012Path).fileName());

    if (a003 == nullptr) {
        failures.append(QStringLiteral("a003-load-failed"));
    }
    if (a011 == nullptr) {
        failures.append(QStringLiteral("a011-load-failed"));
    }
    if (a012 == nullptr) {
        failures.append(QStringLiteral("a012-load-failed"));
    }

    if (failures.isEmpty()) {
        result.insert(QStringLiteral("a003_links"), collectA003Links(a003, failures));
        result.insert(QStringLiteral("a012_destinations"), collectA012Destinations(a012, failures));
        QJsonArray searches;
        searches.append(searchOne(a011, 0, QStringLiteral("A011 PAGE 1"), failures));
        searches.append(searchOne(a011, 1, QStringLiteral("A011 PAGE 2"), failures));
        searches.append(searchOne(a011, 2, QStringLiteral("A011 PAGE 3"), failures));
        result.insert(QStringLiteral("a011_search"), searches);
    }

    if (a003 != nullptr) {
        FPDF_CloseDocument(a003);
    }
    if (a011 != nullptr) {
        FPDF_CloseDocument(a011);
    }
    if (a012 != nullptr) {
        FPDF_CloseDocument(a012);
    }
    FPDF_DestroyLibrary();

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());
    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return failures.isEmpty() ? 0 : 2;
}
