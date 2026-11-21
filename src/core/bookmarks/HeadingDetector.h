#pragma once

// N5.3 Auto Create Bookmarks: finds headings in a book's text lines. Pure C++;
// the reader feeds it one entry per text line with the line's box on the page.
//
// The PDF engine Atlas reads text with (Qt PDF, ADR-0004) gives each line's
// glyph box but not its font, so a line's type size is estimated from the box
// height and which kinds of letters it holds (a line with descenders such as
// "g" or "ي" reaches lower than one without). Headings are lines clearly
// larger than the body text, plus short lines that start like a heading
// ("Chapter 3", "الفصل الثاني", "باب ۲") at body size. Running headers and
// footers (the same text on many pages) and page numbers are left out. Sizes
// that are close together form one level; the largest is level 0.
//
// Everything is a suggestion: the reader sees a preview and chooses.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace atlas::bookmarks {

struct TextLine final {
    int page{};           // 0-based
    std::string text;     // UTF-8, as extracted
    double top{};         // box in page points, y down
    double left{};
    double width{};
    double height{};
    double pageWidth{};
    double pageHeight{};
};

struct HeadingOptions final {
    int maxLevels{3};               // 1–4
    double largerBy{1.12};          // a heading is at least this much larger than body text
    std::size_t maxHeadings{5000};  // preview limit
    std::size_t maxTitleBytes{400}; // longer lines are not headings
};

struct DetectedHeading final {
    int page{};
    std::string title; // UTF-8, whitespace simplified; multi-line headings joined
    double fx{};       // destination as fractions of the page (top-left of the heading)
    double fy{};
    int level{};       // 0 = top
    double size{};     // estimated type size in points
};

struct HeadingResult final {
    std::vector<DetectedHeading> headings;
    double bodySize{};         // estimated body text size in points (0 if no text)
    std::size_t linesRead{};
    bool truncated{};          // more headings than maxHeadings were found
};

// Estimated type size of one line from its box height and letters.
[[nodiscard]] double estimateTypeSize(std::string_view text, double boxHeight);

// True when the line starts like a heading in English, Arabic or Urdu.
[[nodiscard]] bool startsLikeHeading(std::string_view text);

[[nodiscard]] HeadingResult detectHeadings(const std::vector<TextLine>& lines, const HeadingOptions& options = {});

} // namespace atlas::bookmarks
