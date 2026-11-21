// Interface font contracts (src/app/fonts): the bundled Arabic and Urdu fonts
// exist under their own family names in exactly the two weights the interface
// uses, and carry Latin text and direction marks themselves; user content is
// shown without the characters its font lacks until looking them up is cheap.
// Anything less makes Qt on Windows read every installed font before the
// first frame.
#include "app/fonts/InterfaceFonts.h"

#include <QElapsedTimer>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QRawFont>

#include <iostream>
#include <stdexcept>
#include <string>
#include <initializer_list>

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

QString fromCodePoints(std::initializer_list<char32_t> codePoints) {
    QString text;
    for (const char32_t codePoint : codePoints) text.append(QChar::fromUcs4(codePoint));
    return text;
}

void contentIsShownWithoutCharactersItsFontLacks() {
    using atlas::fonts::ContentFonts;
    const QString naskh = QString::fromLatin1(atlas::fonts::arabicFamily);
    // Arabic "kitab", space, CJK angle brackets around a Han character, space, "x".
    const QString arabic = fromCodePoints({0x0643, 0x062A, 0x0627, 0x0628});
    const QString mixed = arabic + fromCodePoints({0x20, 0x300A, 0x6E2C, 0x300B, 0x20, 0x78});
    const QString han = fromCodePoints({0x6E2C, 0x8A66});
    const QString isolated = fromCodePoints({0x2068}) + arabic + fromCodePoints({0x2069});
    const QString emoji = fromCodePoints({0x41, 0x1F600, 0x42});
    const QString emojiForm = fromCodePoints({0x41, 0x00A9, 0xFE0F, 0x42});

    ContentFonts held(ContentFonts::Mode::Hold);
    require(!held.ready(), "hold mode must not be ready");
    held.start(); // no effect in hold mode
    require(!held.ready(), "hold mode became ready");
    require(held.covers(QStringLiteral("Plain title 12 (v2).pdf"), naskh), "ASCII must be covered");
    require(held.covers(arabic, naskh), "Arabic must be covered by the Arabic font");
    require(held.covers(isolated, naskh), "the bundled font carries the isolate marks");
    require(!held.covers(mixed, naskh), "CJK is not in the Arabic font");
    require(!held.covers(emoji, naskh), "emoji always come from another font");
    require(!held.covers(emojiForm, naskh), "a character asked for in emoji form comes from another font");
    require(held.shown(QStringLiteral("Plain title"), naskh) == QStringLiteral("Plain title"), "ASCII changed");
    require(held.shown(arabic, naskh) == arabic, "covered Arabic changed");
    require(held.shown(mixed, naskh) == arabic + fromCodePoints({0x20, 0x20, 0x78}), "uncovered characters were not left out");
    require(held.shown(han, naskh) == QStringLiteral("..."), "a title with nothing drawable must not be empty");
    require(held.shown(emoji, naskh) == QStringLiteral("AB"), "emoji was not left out");
    require(held.shown(emojiForm, naskh) == fromCodePoints({0x41, 0xFE0F, 0x42}), "emoji-form character was not left out");

    ContentFonts off(ContentFonts::Mode::Off);
    require(off.ready(), "off mode is ready at once");
    require(off.shown(mixed, naskh) == mixed, "ready content must be shown unchanged");

    // Idle mode: ready at once where no read is needed, otherwise after the read.
    ContentFonts idle(ContentFonts::Mode::Idle);
    idle.start();
    QElapsedTimer timer;
    timer.start();
    while (!idle.ready() && timer.elapsed() < 60000) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    require(idle.ready(), "idle mode never became ready");
    require(idle.shown(mixed, naskh) == mixed, "content was not restored when ready");
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
        contentIsShownWithoutCharactersItsFontLacks();
        fallbackProbeReturnsADuration();
    } catch (const std::exception& error) {
        std::cerr << "interface_fonts_tests FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "interface_fonts_tests passed\n";
    return 0;
}
