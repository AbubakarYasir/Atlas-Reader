#include "app/fonts/InterfaceFonts.h"

#include <QFont>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QTextLayout>

#include <chrono>

namespace atlas::fonts {

QStringList registerBundledFonts() {
    static const char* const files[] = {"AtlasNaskh-Regular.ttf", "AtlasNaskh-SemiBold.ttf",
                                        "AtlasNastaliq-Regular.ttf", "AtlasNastaliq-SemiBold.ttf"};
    QStringList failed;
    for (const char* name : files) {
        const QString file = QString::fromLatin1(name);
        int id = QFontDatabase::addApplicationFont(QStringLiteral(":/qt/qml/AtlasReader/assets/fonts/") + file);
        if (id < 0) id = QFontDatabase::addApplicationFont(QStringLiteral(":/AtlasReader/assets/fonts/") + file);
        if (id < 0) failed.append(file);
    }
    return failed;
}

namespace {

bool hasExactWeight(const QString& family, int weight) {
    const QStringList styles = QFontDatabase::styles(family);
    for (const QString& style : styles) {
        if (QFontDatabase::weight(family, style) == weight && !QFontDatabase::italic(family, style)) return true;
    }
    return false;
}

} // namespace

bool attachSystemSemibold(const QString& family) {
    if (family.isEmpty()) return false;
    // Reading the family's own styles first matters: a face attached to a
    // family Qt has not read yet would mark it as read and hide its regular face.
    if (hasExactWeight(family, QFont::DemiBold)) return true;
#ifdef Q_OS_WIN
    for (const char* suffix : {" Semibold", " SemiBold", " Demibold", " DemiBold"}) {
        const QString sibling = family + QLatin1String(suffix);
        if (!QFontDatabase::hasFamily(sibling)) continue;
        static_cast<void>(QFontDatabase::styles(sibling)); // reads that one family
        if (hasExactWeight(family, QFont::DemiBold)) return true;
    }
#endif
    return false;
}

double measureFallbackLookupMs(const QString& family) {
    QFont font(family);
    font.setPixelSize(13);
    // Hangul, Devanagari and Thai: three scripts the interface fonts do not carry.
    const char16_t sample[] = {0xD55C, 0xAE00, 0x20, 0x0939, 0x093F, 0x0928, 0x20, 0x0E44, 0x0E17, 0x0E22, 0};
    QElapsedTimer timer;
    timer.start();
    QTextLayout layout(QString::fromUtf16(sample), font);
    layout.beginLayout();
    static_cast<void>(layout.createLine());
    layout.endLayout();
    return static_cast<double>(timer.nsecsElapsed()) / 1.0e6;
}

FontWarmup::~FontWarmup() { stop(); }

void FontWarmup::start() {
#ifdef Q_OS_WIN
    if (thread_.joinable()) return;
    cancel_.store(false);
    thread_ = std::thread([this] {
        // Each call holds Qt's font lock for one family only (well under a
        // millisecond), so the interface thread is never kept waiting.
        const QStringList families = QFontDatabase::families();
        for (const QString& family : families) {
            if (cancel_.load()) return;
            static_cast<void>(QFontDatabase::styles(family));
            familiesRead_.fetch_add(1);
            // Let the interface thread take the font lock between families.
            std::this_thread::yield();
        }
        finished_.store(true);
    });
#else
    finished_.store(true);
#endif
}

void FontWarmup::stop() {
    cancel_.store(true);
    if (thread_.joinable()) thread_.join();
}

} // namespace atlas::fonts
