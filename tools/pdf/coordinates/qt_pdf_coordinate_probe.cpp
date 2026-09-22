#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QModelIndex>
#include <QPdfDocument>
#include <QPdfLink>
#include <QPdfLinkModel>
#include <QPdfSearchModel>
#include <QPointF>
#include <QRectF>
#include <QThread>
#include <QUrl>

#include <cstddef>
#include <cstdio>

namespace {

QJsonObject rectJson(const QRectF& rect)
{
    QJsonObject out;
    out.insert(QStringLiteral("x"), rect.x());
    out.insert(QStringLiteral("y"), rect.y());
    out.insert(QStringLiteral("width"), rect.width());
    out.insert(QStringLiteral("height"), rect.height());
    return out;
}

QJsonArray rectsJson(const QList<QRectF>& rects)
{
    QJsonArray out;
    for (const QRectF& rect : rects) {
        out.append(rectJson(rect));
    }
    return out;
}

bool settleSearch(QPdfSearchModel& model, int expectedCount)
{
    QElapsedTimer timer;
    timer.start();
    int lastCount = model.rowCount(QModelIndex());
    qint64 lastChange = 0;

    while (timer.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        const int count = model.rowCount(QModelIndex());
        if (count != lastCount) {
            lastCount = count;
            lastChange = timer.elapsed();
        }
        if (count == expectedCount && timer.elapsed() >= 200 && timer.elapsed() - lastChange >= 100) {
            return true;
        }
        QThread::msleep(5);
    }
    return false;
}

QJsonArray collectA003Links(QPdfDocument& document, QJsonArray& failures)
{
    QPdfLinkModel model;
    model.setDocument(&document);
    model.setPage(0);

    const int rectangleRole = static_cast<int>(QPdfLinkModel::Role::Rectangle);
    const int urlRole = static_cast<int>(QPdfLinkModel::Role::Url);
    const int pageRole = static_cast<int>(QPdfLinkModel::Role::Page);
    const int locationRole = static_cast<int>(QPdfLinkModel::Role::Location);

    QJsonArray links;
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        const QModelIndex index = model.index(row, 0, QModelIndex());
        const QRectF rect = model.data(index, rectangleRole).toRectF();
        const QPointF location = model.data(index, locationRole).toPointF();

        QJsonObject item;
        item.insert(QStringLiteral("row"), row);
        item.insert(QStringLiteral("source_page"), 0);
        item.insert(QStringLiteral("destination_page"), model.data(index, pageRole).toInt());
        item.insert(QStringLiteral("url"), model.data(index, urlRole).toUrl().toString());
        item.insert(QStringLiteral("atlas_rect"), rectJson(rect));
        item.insert(QStringLiteral("raw_qt_rect"), rectJson(rect));
        item.insert(QStringLiteral("raw_destination_x"), location.x());
        item.insert(QStringLiteral("raw_destination_y"), location.y());
        links.append(item);
    }

    if (links.size() < 2) {
        failures.append(QStringLiteral("a003-link-count-too-small"));
    }
    return links;
}

QJsonObject searchOne(QPdfDocument& document, int page, const QString& query, QJsonArray& failures)
{
    QPdfSearchModel model;
    model.setDocument(&document);
    model.setSearchString(query);

    QJsonObject out;
    out.insert(QStringLiteral("page"), page);
    out.insert(QStringLiteral("query"), query);

    if (!settleSearch(model, 1)) {
        failures.append(QStringLiteral("search-did-not-settle:%1").arg(page));
        out.insert(QStringLiteral("settled"), false);
        return out;
    }

    out.insert(QStringLiteral("settled"), true);
    out.insert(QStringLiteral("hit_count"), model.rowCount(QModelIndex()));

    const QPdfLink link = model.resultAtIndex(0);
    if (!link.isValid() || link.page() != page) {
        failures.append(QStringLiteral("search-hit-page-or-validity:%1").arg(page));
        return out;
    }

    const QSizeF size = document.pagePointSize(page);
    out.insert(QStringLiteral("visible_width_points"), size.width());
    out.insert(QStringLiteral("visible_height_points"), size.height());
    out.insert(QStringLiteral("atlas_rects"), rectsJson(link.rectangles()));
    out.insert(QStringLiteral("raw_qt_rects"), rectsJson(link.rectangles()));
    out.insert(QStringLiteral("raw_location_x"), link.location().x());
    out.insert(QStringLiteral("raw_location_y"), link.location().y());

    if (link.rectangles().isEmpty()) {
        failures.append(QStringLiteral("search-hit-rectangles-empty:%1").arg(page));
    }
    return out;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 3) {
        std::fprintf(stderr, "usage: atlas_qt_pdf_coordinate_probe <A003.pdf> <A011.pdf>\n");
        return 64;
    }

    const QString a003Path = QString::fromLocal8Bit(argv[1]);
    const QString a011Path = QString::fromLocal8Bit(argv[2]);
    QJsonArray failures;

    QPdfDocument a003;
    const auto a003Error = a003.load(a003Path);
    QPdfDocument a011;
    const auto a011Error = a011.load(a011Path);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-coordinate.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("atlas_page_space"), QStringLiteral("effective-visible-page; origin=top-left; x-right; y-down; units=points"));
    result.insert(QStringLiteral("a003_file"), QFileInfo(a003Path).fileName());
    result.insert(QStringLiteral("a011_file"), QFileInfo(a011Path).fileName());

    if (a003Error != QPdfDocument::Error::None) {
        failures.append(QStringLiteral("a003-load-failed"));
    }
    if (a011Error != QPdfDocument::Error::None) {
        failures.append(QStringLiteral("a011-load-failed"));
    }

    if (failures.isEmpty()) {
        result.insert(QStringLiteral("a003_links"), collectA003Links(a003, failures));

        QJsonArray searches;
        searches.append(searchOne(a011, 0, QStringLiteral("A011 PAGE 1"), failures));
        searches.append(searchOne(a011, 1, QStringLiteral("A011 PAGE 2"), failures));
        searches.append(searchOne(a011, 2, QStringLiteral("A011 PAGE 3"), failures));
        result.insert(QStringLiteral("a011_search"), searches);
    }

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());
    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return failures.isEmpty() ? 0 : 2;
}
