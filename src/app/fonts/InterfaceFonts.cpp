#include "app/fonts/InterfaceFonts.h"

#include <QFont>
#include <QFontDatabase>

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
            if ((familiesRead_.load() & 0x1f) == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
