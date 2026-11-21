// Interface font contracts (src/app/fonts): the bundled Arabic and Urdu fonts
// exist under their own family names in exactly the two weights the interface
// uses, and carry Latin text and direction marks themselves. Anything less
// makes Qt on Windows read every installed font before the first frame.
#include "app/fonts/InterfaceFonts.h"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QRawFont>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void bundledFamiliesHaveBothWeights() {
    const QString directory = QStringLiteral(ATLAS_FONT_DIRECTORY);
    for (const char* file : {"AtlasNaskh-Regular.ttf", "AtlasNaskh-SemiBold.ttf", "AtlasNastaliq-Regular.ttf",
                             "AtlasNastaliq-SemiBold.ttf"}) {
        const int id = QFontDatabase::addApplicationFont(directory + QLatin1Char('/') + QLatin1String(file));
        require(id >= 0, std::string("font not registered: ") + file);
    }
    for (const char* name : {atlas::fonts::arabicFamily, atlas::fonts::urduFamily}) {
        const QString family = QString::fromLatin1(name);
        require(QFontDatabase::hasFamily(family), std::string("family missing: ") + name);
        bool regular = false;
        bool semibold = false;
        for (const QString& style : QFontDatabase::styles(family)) {
            const int weight = QFontDatabase::weight(family, style);
            require(!QFontDatabase::italic(family, style), std::string("unexpected italic style in ") + name);
            regular = regular || weight == QFont::Normal;
            semibold = semibold || weight == QFont::DemiBold;
        }
        require(regular, std::string("no weight 400 in ") + name);
        require(semibold, std::string("no weight 600 in ") + name);
        require(atlas::fonts::attachSystemSemibold(family), std::string("weight 600 is not exact for ") + name);
    }
}

void bundledFontsCarryLatinAndDirectionMarks() {
    // Latin, punctuation, the direction marks I18n.qml inserts, Arabic beh, Urdu yeh.
    const char32_t needed[] = {0x41,   0x7A,   0x30,   0x25,   0x28,   0x2F,   0x2B,   0x2D,   0xB7,   0x2014,
                               0x2026, 0x203A, 0x2022, 0x200E, 0x200F, 0x202A, 0x202B, 0x202C, 0x0628, 0x06CC};
    for (const char* name : {atlas::fonts::arabicFamily, atlas::fonts::urduFamily}) {
        for (const int weight : {QFont::Normal, QFont::DemiBold}) {
            QFont font(QString::fromLatin1(name));
            font.setWeight(static_cast<QFont::Weight>(weight));
            font.setStyleStrategy(QFont::NoFontMerging);
            const QRawFont raw = QRawFont::fromFont(font);
            require(raw.isValid(), std::string("no raw font for ") + name);
            require(raw.familyName().startsWith(QString::fromLatin1(name)),
                    std::string("another font answered for ") + name + ": " + raw.familyName().toStdString());
            for (const char32_t character : needed) {
                require(raw.supportsCharacter(static_cast<uint>(character)),
                        std::string(name) + " lacks U+" + QString::number(static_cast<uint>(character), 16).toStdString());
            }
        }
    }
}

void warmupStopsOnRequest() {
    atlas::fonts::FontWarmup warmup;
    warmup.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    warmup.stop();  // must return promptly whether or not every family was read
    warmup.start(); // a second start after a stop is allowed
    warmup.stop();
}

void fallbackProbeReturnsADuration() {
    require(atlas::fonts::measureFallbackLookupMs(QString::fromLatin1(atlas::fonts::arabicFamily)) >= 0.0,
            "the fallback probe returned a negative duration");
}

} // namespace

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    try {
        bundledFamiliesHaveBothWeights();
        bundledFontsCarryLatinAndDirectionMarks();
        warmupStopsOnRequest();
        fallbackProbeReturnsADuration();
    } catch (const std::exception& error) {
        std::cerr << "interface_fonts_tests FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "interface_fonts_tests passed\n";
    return 0;
}
