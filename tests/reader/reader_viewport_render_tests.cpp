// N4.2 integration test: Qt PDF render worker + ReaderViewport in a real
// (offscreen, software-rendered) Qt Quick window. Generates its own PDF, so it
// needs no fixture files. Verifies that pages actually appear, navigation and
// zoom work, the source file is never locked and its bytes never change.

#include "app/reader/ReaderController.h"
#include "app/reader/ReaderViewport.h"

#include <QColor>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QFontDatabase>
#include <QClipboard>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QUrl>

#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <psapi.h>
#endif

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool waitFor(const std::function<bool()>& condition, int timeoutMs = 10000) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (condition()) return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return condition();
}

void settle(int ms) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

QByteArray sha256(const QString& path) {
    QFile file{path};
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

const int kPages = qEnvironmentVariableIsSet("ATLAS_LONG_BOOK") ? 2000 : 60;

// The offscreen platform on Windows may see no system fonts, which would
// leave the generated book without text; load Arial explicitly when present.
QFont bookFont() {
    static const QString family = [] {
        const int id = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/arial.ttf"));
        return id >= 0 ? QFontDatabase::applicationFontFamilies(id).value(0) : QString{};
    }();
    return family.isEmpty() ? QFont{} : QFont{family};
}

void writeBook(const QString& path) {
    QPdfWriter writer{path};
    writer.setPageSize(QPageSize{QPageSize::A4});
    writer.setResolution(72);
    QPainter painter{&writer};
    for (int page = 1; page <= kPages; ++page) {
        if (page > 1) writer.newPage();
        // A solid red block near the top-left of every page is easy to detect.
        painter.fillRect(QRect{40, 40, 160, 160}, QColor{220, 20, 20});
        QFont font = bookFont();
        font.setPixelSize(48);
        painter.setFont(font);
        painter.drawText(QRect{0, 300, 595, 80}, Qt::AlignCenter, QStringLiteral("Page %1").arg(page));
    }
}

int countRed(const QImage& image) {
    int red = 0;
    for (int y = 0; y < image.height(); y += 2) {
        for (int x = 0; x < image.width(); x += 2) {
            const QColor c = image.pixelColor(x, y);
            if (c.red() > 180 && c.green() < 80 && c.blue() < 80) ++red;
        }
    }
    return red;
}

} // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    QGuiApplication app{argc, argv};

    try {
        QTemporaryDir dir;
        require(dir.isValid(), "A temporary directory is required.");
        const QString source = dir.filePath(QStringLiteral("long book.pdf"));
        writeBook(source);
        const QByteArray before = sha256(source);
        require(!before.isEmpty(), "The generated PDF must exist.");

        atlas::app::ReaderController controller{
            std::filesystem::path{dir.filePath(QStringLiteral("profile")).toStdWString()}};
        controller.openLocalFile(QUrl::fromLocalFile(source));
        require(waitFor([&] {
                    return controller.data(controller.index(0), atlas::app::ReaderController::StatusRole)
                               .toString() == QStringLiteral("ready");
                }),
                "The session must reach ready state.");
        const auto sessionId =
            controller.data(controller.index(0), atlas::app::ReaderController::SessionIdRole)
                .toULongLong();

        QQuickWindow window;
        window.resize(800, 600);
        window.setColor(QColor{233, 230, 222});
        // Offset the viewport so placement and clipping in item space are tested.
        auto* header = new QQuickItem{window.contentItem()};
        header->setSize({800, 120});
        auto* viewport = new atlas::app::ReaderViewport{window.contentItem()};
        viewport->setPosition({0, 120});
        viewport->setSize({800, 480});
        viewport->setController(&controller);
        viewport->setSessionId(sessionId);
        viewport->setActive(true);
        window.show();

        require(waitFor([&] { return viewport->pageCount() == kPages && !viewport->loading(); }),
                "The viewport must load every page geometry.");
        require(viewport->errorKey().isEmpty(), "Opening for rendering must not fail.");
        require(viewport->fitMode() == atlas::app::ReaderViewport::FitMode::Width &&
                    viewport->zoom() > 0.5 && viewport->zoom() < 2.0,
                "A new document opens fitted to the width.");
        require(viewport->currentPage() == 1 && viewport->contentY() == 0.0,
                "A new document opens at its first page.");
        require(viewport->contentHeight() > kPages * 480.0,
                "Content height must cover every page.");

        settle(800);
        QImage frame = window.grabWindow();
        const int redFirst = countRed(frame.copy(0, 120, 800, 480));
        require(redFirst > 500, "The first page must be rendered inside the viewport.");
        require(countRed(frame.copy(0, 0, 800, 120)) == 0,
                "Rendering must stay clipped to the viewport (nothing drawn over the header).");

        viewport->goToPage(45);
        require(viewport->currentPage() == 45, "Jumping must land on the requested page.");
        settle(800);
        frame = window.grabWindow();
        require(countRed(frame.copy(0, 120, 800, 480)) > 500, "The jumped-to page must render.");

        viewport->zoomBy(2.0);
        require(viewport->fitMode() == atlas::app::ReaderViewport::FitMode::None,
                "A manual zoom leaves fit mode.");
        require(viewport->currentPage() == 45, "Zooming must keep the current page in view.");

        viewport->setFitMode(atlas::app::ReaderViewport::FitMode::Page);
        settle(500);
        require(viewport->currentPage() == 45 && viewport->zoom() < 1.0,
                "Fit page must show the whole current page.");

        viewport->goToPage(kPages + 10);
        require(viewport->currentPage() == kPages, "Jumping past the end clamps to the last page.");
        require(viewport->canGoBack(), "Page jumps must be recorded for Previous View.");
        viewport->goBack();
        require(viewport->currentPage() == 45, "Previous View must return to the page before the jump.");
        require(viewport->canGoForward(), "Previous View must enable Next View.");
        viewport->goForward();
        require(viewport->currentPage() == kPages, "Next View must return to the jump target.");
        viewport->nextPage();
        viewport->previousPage();
        viewport->goBack();
        require(viewport->currentPage() == 45, "Next/previous page must not be recorded as jumps.");

        viewport->setPageDisplay(0); // Single Page
        viewport->goToPage(10);
        for (int i = 0; i < 40 && viewport->currentPage() == 10; ++i) viewport->scrollByPages(1.0);
        require(viewport->currentPage() == 11, "Single Page mode must turn to the next page at the end of a page.");
        viewport->setPageDisplay(2); // Facing
        viewport->goToPage(1);
        viewport->nextPage();
        require(viewport->currentPage() == 3, "Facing mode must turn a whole spread.");
        viewport->setCoverPage(true);
        viewport->goToPage(1);
        viewport->nextPage();
        require(viewport->currentPage() == 2, "With a cover page the first spread starts at page 2.");
        viewport->setPageDisplay(1);
        viewport->setCoverPage(false);

        // N4.5 text: info, search (incl. whole words), selection, word count.
        require(waitFor([&] { return viewport->textReady(); }), "Document text information must arrive.");
        require(viewport->hasText(), "Document text must be detected.");
        require(viewport->pageLabels().size() == kPages,
                "Page labels must be read (got " + std::to_string(viewport->pageLabels().size()) + ").");
        if (kPages >= 1000) {
#ifdef Q_OS_WIN
            // Actual Windows private committed bytes during repeated navigation,
            // including render/text workers, not just a cache bookkeeping counter.
            const auto privateBytes = [] {
                PROCESS_MEMORY_COUNTERS_EX counters{};
                require(GetProcessMemoryInfo(GetCurrentProcess(),
                            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)) != 0,
                        "Windows memory measurement succeeds.");
                return static_cast<std::size_t>(counters.PrivateUsage);
            };
            std::vector<std::size_t> memory;
            for (int batch = 0; batch < 8; ++batch) {
                for (int jump = 0; jump < 50; ++jump) {
                    viewport->goToPage(1 + ((batch * 50 + jump) * 17) % kPages);
                    settle(35);
                }
                settle(150);
                memory.push_back(privateBytes());
            }
            std::cout << "2000-page navigation private MiB (50 jumps per sample):";
            for (const auto bytes : memory) std::cout << ' ' << bytes / (1024 * 1024);
            std::cout << '\n';
            const auto latePeak = *std::max_element(memory.begin() + 4, memory.end());
            require(latePeak <= memory[3] + 96ULL * 1024ULL * 1024ULL,
                    "After 200 warm-up jumps, 200 more jumps stay within 96 MiB growth.");
#endif
            std::vector<double> samples;
            for (int i = 0; i < 100; ++i) {
                QElapsedTimer measurement;
                measurement.start();
                viewport->find(QStringLiteral("Page %1").arg(1 + (i * 97) % kPages), false, true);
                require(waitFor([&] { return !viewport->searching(); }, 30000), "Long-book search completes.");
                require(viewport->searchHits().size() == 1, "Long-book unique hit.");
                samples.push_back(static_cast<double>(measurement.nsecsElapsed()) / 1000000.0);
            }
            std::sort(samples.begin(), samples.end());
            std::cout << "Search 2000 pages, 100 warm sequential queries: p50_ms=" << samples[49]
                      << " p95_ms=" << samples[94] << " p99_ms=" << samples[98] << '\n';
            viewport->find(QStringLiteral("Page"), false, false);
            viewport->clearSearch();
            settle(100);
            require(!viewport->searching() && viewport->searchHits().isEmpty(), "Cancelled long search stays empty.");
        }
        viewport->find(QStringLiteral("page 42"), false, true);
        require(waitFor([&] { return !viewport->searching(); }), "Search must finish.");
        require(viewport->searchHits().size() == 1 && viewport->currentPage() == 42,
                "Search must find the single match and show its page.");
        viewport->find(QStringLiteral("PAGE"), false, false);
        require(waitFor([&] { return !viewport->searching(); }), "Search must finish.");
        require(viewport->searchHits().size() == kPages, "Case-insensitive search must find every page.");
        viewport->find(QStringLiteral("PAGE"), true, false);
        require(waitFor([&] { return !viewport->searching(); }) && viewport->searchHits().isEmpty(),
                "Match Case must not match different case.");
        viewport->clearSearch();
        viewport->goToPage(7);
        viewport->selectAllOnPage();
        require(waitFor([&] { return viewport->selectedText().contains(QStringLiteral("Page 7")); }),
                "Selecting a page must return its text.");
        int words = -1;
        QObject::connect(viewport, &atlas::app::ReaderViewport::wordCountReady, viewport,
                         [&words](int w, int, int) { words = w; });
        viewport->requestWordCount();
        require(waitFor([&] { return words >= 0; }) && words == kPages * 2, "Word Count must count every word.");
        require(viewport->goToLabel(QStringLiteral("33")) && viewport->currentPage() == 33,
                "Going to a page by label or number must work.");
        require(waitFor([&] { return viewport->pageText().contains(QStringLiteral("Page 33")); }),
                "Screen readers must get the current page's text.");
        // Garbage destinations (NaN, off-page) fall back to the top of the target page.
        viewport->goToDestination(4, QPointF{std::nan(""), 1e30});
        require(viewport->currentPage() == 5, "An unusable destination must still reach its page.");

        // SnapShot copies the page as drawn, without the blue selection box
        // (owner report 2026-11-08: every snapshot had the box's tint).
        {
            viewport->setTool(atlas::app::ReaderViewport::SnapshotTool);
            settle(300);
            bool copied = false;
            QObject::connect(viewport, &atlas::app::ReaderViewport::snapshotCopied, [&] { copied = true; });
            const QPointF from{200, 220}, to{600, 420}; // window coordinates, inside the viewport
            auto send = [&](QEvent::Type type, QPointF at, Qt::MouseButtons buttons) {
                QMouseEvent event{type, at, at, window.mapToGlobal(at), Qt::LeftButton, buttons, Qt::NoModifier};
                QGuiApplication::sendEvent(&window, &event);
            };
            send(QEvent::MouseButtonPress, from, Qt::LeftButton);
            for (int step = 1; step <= 8; ++step) {
                send(QEvent::MouseMove, from + (to - from) * (step / 8.0), Qt::LeftButton);
                settle(20);
            }
            send(QEvent::MouseButtonRelease, to, Qt::NoButton);
            require(waitFor([&] { return copied; }), "SnapShot must copy an image.");
            const QImage snap = QGuiApplication::clipboard()->image().convertToFormat(QImage::Format_RGB32);
            require(snap.width() >= 395 && snap.height() >= 195, "SnapShot must copy the dragged area.");
            settle(200);
            const QImage clean = window.grabWindow().copy(QRect{QPoint{200, 220}, snap.size()})
                                     .convertToFormat(QImage::Format_RGB32);
            int differing = 0;
            for (int y = 0; y < snap.height(); ++y)
                for (int x = 0; x < snap.width(); ++x)
                    if (snap.pixel(x, y) != clean.pixel(x, y)) ++differing;
            require(differing < snap.width() * snap.height() / 100,
                    "SnapShot must not include the selection box (copied image differs from the page).");
            viewport->setTool(atlas::app::ReaderViewport::HandTool);
        }

        // The renderer holds the document in memory, not the file.
        const QString moved = dir.filePath(QStringLiteral("moved book.pdf"));
        require(QFile::rename(source, moved), "Atlas must not block renaming an open PDF.");
        require(sha256(moved) == before, "Reading must never change the PDF's bytes.");

        viewport->setActive(false);
        settle(200);
        delete viewport;
        settle(100);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "reader viewport render tests passed\n";
    return 0;
}
