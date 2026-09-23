#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfDocument>
#include <QPdfSearchModel>
#include <QSize>
#include <QThread>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

namespace {

bool settleSearch(QPdfSearchModel& model)
{
    constexpr int timeoutMs = 2500;
    constexpr int minimumObservationMs = 350;
    constexpr int stableWindowMs = 120;
    QElapsedTimer timer;
    timer.start();
    int lastCount = model.rowCount(QModelIndex());
    qint64 lastChange = 0;
    while (timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        const int count = model.rowCount(QModelIndex());
        if (count != lastCount) {
            lastCount = count;
            lastChange = timer.elapsed();
        }
        if (timer.elapsed() >= minimumObservationMs && timer.elapsed() - lastChange >= stableWindowMs) {
            return true;
        }
        QThread::msleep(5);
    }
    return false;
}

QJsonObject renderOne(QPdfDocument& document, int scale, const QString& outputPath, QJsonArray& failures)
{
    const QSizeF points = document.pagePointSize(0);
    const QSize pixels(
        (std::max)(1, static_cast<int>(std::lround(points.width() * scale))),
        (std::max)(1, static_cast<int>(std::lround(points.height() * scale))));
    const QImage image = document.render(0, pixels);

    QJsonObject result;
    result.insert(QStringLiteral("scale"), scale);
    result.insert(QStringLiteral("width"), image.width());
    result.insert(QStringLiteral("height"), image.height());
    result.insert(QStringLiteral("is_null"), image.isNull());
    result.insert(QStringLiteral("png"), outputPath);

    if (image.isNull()) {
        failures.append(QStringLiteral("render-null:%1x").arg(scale));
    } else if (!image.save(outputPath, "PNG")) {
        failures.append(QStringLiteral("png-save-failed:%1x").arg(scale));
    }
    return result;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        std::fprintf(stderr, "usage: atlas_qt_pdf_content_probe <fixture-id> <pdf> <output-dir>\n");
        return 64;
    }

    const QString fixtureId = QString::fromLocal8Bit(argv[1]);
    const QString pdfPath = QString::fromLocal8Bit(argv[2]);
    const QString outputDir = QString::fromLocal8Bit(argv[3]);
    QDir().mkpath(outputDir);

    QJsonObject result;
    result.insert(QStringLiteral("schema"), QStringLiteral("atlas.n2.qt-pdf-content-fidelity.v1"));
    result.insert(QStringLiteral("engine"), QStringLiteral("qt-pdf"));
    result.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("fixture_id"), fixtureId);
    result.insert(QStringLiteral("file_name"), QFileInfo(pdfPath).fileName());

    QJsonArray failures;
    QPdfDocument document;
    const auto error = document.load(pdfPath);
    result.insert(QStringLiteral("load_error"), static_cast<int>(error));
    if (error != QPdfDocument::Error::None) {
        failures.append(QStringLiteral("load-failed"));
    }

    if (failures.isEmpty()) {
        result.insert(QStringLiteral("page_count"), document.pageCount());
        if (document.pageCount() != 1) {
            failures.append(QStringLiteral("page-count"));
        } else {
            const QSizeF points = document.pagePointSize(0);
            result.insert(QStringLiteral("width_points"), points.width());
            result.insert(QStringLiteral("height_points"), points.height());

            const QString text = document.getAllText(0).text();
            result.insert(QStringLiteral("text_utf16_length"), text.size());

            QPdfSearchModel search;
            search.setDocument(&document);
            search.setSearchString(QStringLiteral("Atlas"));
            const bool settled = settleSearch(search);
            result.insert(QStringLiteral("search_settled"), settled);
            result.insert(QStringLiteral("search_hit_count"), search.rowCount(QModelIndex()));
            if (!settled) {
                failures.append(QStringLiteral("search-timeout"));
            }

            QJsonArray renders;
            for (int scale : {1, 2}) {
                const QString png = QDir(outputDir).filePath(
                    QStringLiteral("qt-%1-%2x.png").arg(fixtureId).arg(scale));
                renders.append(renderOne(document, scale, png, failures));
            }
            result.insert(QStringLiteral("renders"), renders);
        }
    }

    result.insert(QStringLiteral("failures"), failures);
    result.insert(QStringLiteral("passed"), failures.isEmpty());
    const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Indented);
    std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    return failures.isEmpty() ? 0 : 2;
}
