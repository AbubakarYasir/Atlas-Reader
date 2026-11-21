#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QRawFont>
#include <QString>
#include <QStringList>
#include <QTimer>

// Interface fonts and the measures that keep Windows from reading every
// installed font before the first frame (N1-03; docs/DESIGN.md 4.2).
//
// On Windows Qt reads a font family only when it is first used, but reads ALL
// families on the interface thread (about half a second with two thousand
// installed, seconds when it happens twice) as soon as
//   1. a request is not an exact match (a weight the family lacks), or
//   2. a piece of text has a character its font lacks.
//
// Interface text never does either: the bundled fonts carry Latin,
// punctuation and direction marks in both weights the interface uses, and the
// system font gets its semibold face attached before any text is laid out.
//
// User content (book titles, file names, bookmarks) can contain any
// character. ContentFonts handles that: after the first frame it reads the
// remaining families in short slices while the interface is idle, and until
// that is done user content is shown without the characters its font lacks.
// Those characters appear a moment later, once looking them up is cheap.
namespace atlas::fonts {

inline constexpr auto arabicFamily = "Atlas Naskh";
inline constexpr auto urduFamily = "Atlas Nastaliq";

// Registers the bundled fonts. Returns the files that could not be registered.
[[nodiscard]] QStringList registerBundledFonts();

// Makes weight 600 an exact match for `family`. Windows lists "Segoe UI
// Semibold" as a separate family; reading that one family attaches its face
// to "Segoe UI". Returns true when the family has an exact weight-600 style.
bool attachSystemSemibold(const QString& family);

// Diagnostic: lays out a short text with characters from scripts the family
// lacks and returns how long Qt's lookup of other fonts took (milliseconds).
[[nodiscard]] double measureFallbackLookupMs(const QString& family);

class ContentFonts : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)

public:
    enum class Mode {
        Idle, // read the remaining families in idle slices, then become ready (default)
        Off,  // ready at once; Qt reads families on demand (ATLAS_FONT_WARMUP=0)
        Hold, // never read and never become ready (ATLAS_FONT_WARMUP=hold, for the CI font check)
    };

    explicit ContentFonts(Mode mode, QObject* parent = nullptr);

    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] int familiesRead() const noexcept { return familiesRead_; }

    // Call once the first frame is on screen.
    void start();

    // `text` as it can be shown in `family` right now: unchanged once ready,
    // or when every character is in the family's own font; otherwise without
    // the characters that would send Qt looking through other fonts.
    Q_INVOKABLE QString shown(const QString& text, const QString& family);

    // True when laying out `text` in `family` needs no other font.
    [[nodiscard]] bool covers(const QString& text, const QString& family);

signals:
    void readyChanged();

private:
    void step();
    [[nodiscard]] bool needsOtherFont(char32_t character, const QString& family);

    Mode mode_;
    bool ready_ = false;
    bool started_ = false;
    int familiesRead_ = 0;
    qsizetype next_ = 0;
    QStringList families_;
    QTimer timer_;
    QHash<QString, QRawFont> rawFonts_;
    QHash<QString, QHash<char32_t, bool>> needsOther_;
};

} // namespace atlas::fonts
