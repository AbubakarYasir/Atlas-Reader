// N5.3 Auto Create Bookmarks, end to end on the reader's text thread: a book
// generated here (QPdfWriter) with chapters, sections, body text, a running
// header and page numbers. Qt PDF reads the lines; the core detector suggests
// headings; the scan runs in slices so other text requests are answered while
// it runs.

#include "app/reader/DocumentTextWorker.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QVariantMap>

#include <atomic>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool waitFor(const std::function<bool()>& condition, int timeoutMs = 60000) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (condition()) return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return condition();
}

// The offscreen platform on Windows may see no system fonts; load Arial.
QFont bookFont(int points, bool bold = false) {
    static const QString family = [] {
        const int id = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/arial.ttf"));
        return id >= 0 ? QFontDatabase::applicationFontFamilies(id).value(0) : QString{};
    }();
    QFont font = family.isEmpty() ? QFont{} : QFont{family};
    font.setPixelSize(points); // 72 dpi: pixels are points
    font.setBold(bold);
    return font;
}

constexpr int kChapters = 12;
constexpr int kPagesPerChapter = 5;

QByteArray writeBook() {
    QByteArray bytes;
    {
        QBuffer buffer{&bytes};
        buffer.open(QIODevice::WriteOnly);
        QPdfWriter writer{&buffer};
        writer.setPageSize(QPageSize{QPageSize::A4});
        writer.setResolution(72);
        writer.setPageMargins({});
        QPainter painter{&writer};
        const QString body = QStringLiteral("The quick brown fox jumps over the lazy dog, again and again, quietly.");
        int page = 0;
        for (int chapter = 1; chapter <= kChapters; ++chapter) {
            for (int p = 0; p < kPagesPerChapter; ++p, ++page) {
                if (page > 0) writer.newPage();
                painter.setFont(bookFont(9));
                painter.drawText(72, 30, QStringLiteral("A Generated Book for Testing"));
                painter.drawText(290, 815, QString::number(page + 1));
                int y = 80;
                if (p == 0) {
                    painter.setFont(bookFont(22, true));
                    painter.drawText(72, y += 40, QStringLiteral("Chapter %1 Getting Started").arg(chapter));
                }
                if (p == 0 || p == 2) {
                    painter.setFont(bookFont(15, true));
                    painter.drawText(72, y += 36, QStringLiteral("Section %1.%2 Methods").arg(chapter).arg(p == 0 ? 1 : 2));
                }
                painter.setFont(bookFont(11));
                for (int line = 0; line < 30; ++line) painter.drawText(72, y += 16, body);
            }
        }
    }
    return bytes;
}

} // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app{argc, argv};
    try {
        const QByteArray book = writeBook();
        require(book.size() > 1000, "The generated book must exist.");

        auto latestSearch = std::make_shared<std::atomic<quint64>>(0);
        atlas::app::DocumentTextWorker worker{latestSearch};
        bool opened = false;
        QObject::connect(&worker, &atlas::app::DocumentTextWorker::info, [&](quint64, const QVariantList&,
                                                                              const QStringList&, bool hasText) {
            opened = hasText;
        });
        worker.open(book, {}, 1);
        require(waitFor([&] { return opened; }), "The book opens with text.");

        QVariantList headings;
        double bodySize = 0;
        bool done = false;
        int progressCalls = 0;
        bool answeredDuringScan = false;
        QObject::connect(&worker, &atlas::app::DocumentTextWorker::headingsProgress,
                         [&](quint64 id, int, int) { if (id == 7) ++progressCalls; });
        QObject::connect(&worker, &atlas::app::DocumentTextWorker::headingsReady,
                         [&](quint64 id, const QVariantList& found, double body, bool) {
                             if (id != 7) return;
                             headings = found;
                             bodySize = body;
                             done = true;
                         });
        bool answered = false;
        QObject::connect(&worker, &atlas::app::DocumentTextWorker::contentBoxReady,
                         [&](quint64, int, const QRectF&) { answered = true; if (!done) answeredDuringScan = true; });

        QElapsedTimer timer;
        timer.start();
        worker.scanHeadings(7, 3);
        // Another text request while the scan runs is answered between slices.
        QMetaObject::invokeMethod(&worker, [&worker] { worker.contentBox(1, 0); }, Qt::QueuedConnection);
        require(waitFor([&] { return done; }), "The scan finishes.");
        const auto elapsed = timer.elapsed();
        require(waitFor([&] { return answered; }, 5000), "Other text requests are still answered.");
        // A scan longer than one slice lets the queued request run before it ends.
        if (elapsed > 200) require(answeredDuringScan, "Queued text work runs between scan slices.");

        std::cout << "Scanned " << kChapters * kPagesPerChapter << " pages in " << elapsed << " ms; body "
                  << bodySize << " pt; " << headings.size() << " headings; progress updates " << progressCalls << "\n";
        require(std::abs(bodySize - 11) <= 1.0, "Body text is about 11 pt, got " + std::to_string(bodySize));
        require(headings.size() == kChapters * 3,
                "Expected " + std::to_string(kChapters * 3) + " headings, got " + std::to_string(headings.size()));
        for (int chapter = 0; chapter < kChapters; ++chapter) {
            const QVariantMap c = headings[chapter * 3].toMap();
            const QVariantMap s1 = headings[chapter * 3 + 1].toMap();
            const QVariantMap s2 = headings[chapter * 3 + 2].toMap();
            require(c.value("title").toString() == QStringLiteral("Chapter %1 Getting Started").arg(chapter + 1) &&
                        c.value("level").toInt() == 0 && c.value("page").toInt() == chapter * kPagesPerChapter,
                    "Chapter heading at level 0 on its page: " + c.value("title").toString().toStdString());
            require(s1.value("level").toInt() == 1 && s2.value("level").toInt() == 1 &&
                        s2.value("page").toInt() == chapter * kPagesPerChapter + 2,
                    "Sections at level 1: " + s2.value("title").toString().toStdString());
            const double fy = c.value("fy").toDouble();
            require(fy > 0.08 && fy < 0.16, "The destination is at the heading, fy " + std::to_string(fy));
        }

        // Cancelling: a cancelled scan never answers.
        done = false;
        worker.scanHeadings(8, 3);
        worker.cancelScan();
        bool lateAnswer = false;
        QObject::connect(&worker, &atlas::app::DocumentTextWorker::headingsReady,
                         [&](quint64 id, const QVariantList&, double, bool) { if (id == 8) lateAnswer = true; });
        QElapsedTimer settle;
        settle.start();
        while (settle.elapsed() < 500) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        require(!lateAnswer, "A cancelled scan stays silent.");
    } catch (const std::exception& error) {
        std::cerr << "Heading scan test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Heading scan passed.\n";
    return 0;
}
