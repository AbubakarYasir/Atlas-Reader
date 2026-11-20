#pragma once

#include <QString>
#include <QStringList>

#include <atomic>
#include <thread>

// Interface fonts and the two measures that keep Windows from reading every
// installed font before the first frame (N1-03; docs/DESIGN.md "Fonts").
//
// On Windows Qt reads a font family only when it is first used, but reads ALL
// families (seconds, with thousands installed) as soon as
//   1. a request is not an exact match (a weight the family lacks), or
//   2. a piece of text has a character its font lacks.
// The interface is therefore built so that neither happens at startup:
// the bundled fonts carry Latin, punctuation and direction marks in both
// weights the interface uses, and the system font gets its semibold face
// attached before any text is laid out.
namespace atlas::fonts {

inline constexpr auto arabicFamily = "Atlas Naskh";
inline constexpr auto urduFamily = "Atlas Nastaliq";

// Registers the bundled fonts. Returns the files that could not be registered.
[[nodiscard]] QStringList registerBundledFonts();

// Makes weight 600 an exact match for `family`. Windows lists "Segoe UI
// Semibold" as a separate family; reading that one family attaches its face
// to "Segoe UI". Returns true when the family has an exact weight-600 style.
bool attachSystemSemibold(const QString& family);

// Reads the remaining font families one at a time on a background thread,
// after the first frame, so that a later character from another script costs
// milliseconds instead of a pause. Windows only; a no-op elsewhere.
class FontWarmup {
public:
    FontWarmup() = default;
    ~FontWarmup();
    FontWarmup(const FontWarmup&) = delete;
    FontWarmup& operator=(const FontWarmup&) = delete;

    void start();
    void stop();
    [[nodiscard]] bool finished() const noexcept { return finished_.load(); }
    [[nodiscard]] int familiesRead() const noexcept { return familiesRead_.load(); }

private:
    std::thread thread_;
    std::atomic<bool> cancel_{false};
    std::atomic<bool> finished_{false};
    std::atomic<int> familiesRead_{0};
};

} // namespace atlas::fonts
