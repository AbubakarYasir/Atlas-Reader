#include "app/fonts/InterfaceFonts.h"

#include <QFont>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QTextLayout>


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

ContentFonts::ContentFonts(Mode mode, QObject* parent) : QObject(parent), mode_(mode) {
#ifndef Q_OS_WIN
    // Other platforms resolve missing characters without reading every font.
    if (mode_ == Mode::Idle) mode_ = Mode::Off;
#endif
    ready_ = mode_ == Mode::Off;
    timer_.setInterval(0);
    connect(&timer_, &QTimer::timeout, this, &ContentFonts::step);
}

void ContentFonts::start() {
    if (started_ || mode_ != Mode::Idle) return;
    started_ = true;
    families_ = QFontDatabase::families();
    timer_.start();
}

void ContentFonts::step() {
    // A few milliseconds at a time, between events, so input and animation
    // are never held up for longer than one slice.
    QElapsedTimer slice;
    slice.start();
    while (next_ < families_.size() && slice.nsecsElapsed() < 2'000'000) {
        static_cast<void>(QFontDatabase::styles(families_.at(next_))); // reads that one family
        ++next_;
        ++familiesRead_;
    }
    if (next_ < families_.size()) return;
    timer_.stop();
    families_.clear();
    ready_ = true;
    emit readyChanged();
}

bool ContentFonts::needsOtherFont(char32_t character, const QString& family) {
    // Characters Qt never looks up in another font (QFontEngine::isIgnorableChar).
    if (character == 0x2028 || character == 0x2029 || (character & 0xFFFFFFF0U) == 0xFE00
        || QChar::category(character) == QChar::Other_Control) {
        return false;
    }
    // Emoji are drawn from a colour font whatever the family has.
    if (character >= 0x1F000 || (character >= 0x2600 && character <= 0x27BF) || (character >= 0x2B00 && character <= 0x2BFF)
        || character == 0x20E3) {
        return true;
    }
    auto& known = needsOther_[family];
    if (const auto it = known.constFind(character); it != known.constEnd()) return it.value();
    auto fontIt = rawFonts_.find(family);
    if (fontIt == rawFonts_.end()) {
        QFont font(family);
        font.setPixelSize(13);
        font.setStyleStrategy(QFont::NoFontMerging); // the family's own font, nothing else
        fontIt = rawFonts_.insert(family, QRawFont::fromFont(font));
    }
    const bool needs = !fontIt->isValid() || !fontIt->supportsCharacter(static_cast<uint>(character));
    known.insert(character, needs);
    return needs;
}

bool ContentFonts::covers(const QString& text, const QString& family) {
    const QList<uint> characters = text.toUcs4();
    for (qsizetype i = 0; i < characters.size(); ++i) {
        // U+FE0F asks for the emoji form of the character before it.
        const bool emojiForm = i + 1 < characters.size() && characters.at(i + 1) == 0xFE0F;
        if (emojiForm || needsOtherFont(static_cast<char32_t>(characters.at(i)), family)) return false;
    }
    return true;
}

QString ContentFonts::shown(const QString& text, const QString& family) {
    if (ready_ || text.isEmpty() || family.isEmpty()) return text;
    bool allAscii = true;
    for (const QChar unit : text) {
        if (unit.unicode() < 0x20 || unit.unicode() > 0x7E) {
            allAscii = false;
            break;
        }
    }
    if (allAscii) return text; // every interface font has printable ASCII
    QString kept;
    kept.reserve(text.size());
    bool dropped = false;
    const QList<uint> characters = text.toUcs4();
    for (qsizetype i = 0; i < characters.size(); ++i) {
        const bool emojiForm = i + 1 < characters.size() && characters.at(i + 1) == 0xFE0F;
        if (emojiForm || needsOtherFont(static_cast<char32_t>(characters.at(i)), family)) {
            dropped = true;
        } else {
            kept.append(QChar::fromUcs4(static_cast<char32_t>(characters.at(i))));
        }
    }
    if (!dropped) return text;
    if (kept.trimmed().isEmpty()) return QStringLiteral("...");
    return kept;
}

} // namespace atlas::fonts
