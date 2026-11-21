// N5.3 Auto Create Bookmarks: heading detection from text lines (pure C++).

#include "core/bookmarks/HeadingDetector.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace atlas::bookmarks;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

// A line as the reader would report it: box height from the type size and
// which glyphs the text holds (the inverse of estimateTypeSize).
TextLine line(int page, std::string text, double size, double top, double width = 470) {
    TextLine l;
    l.page = page;
    l.text = std::move(text);
    l.top = top;
    l.left = 72;
    l.width = width;
    l.pageWidth = 595;
    l.pageHeight = 842;
    l.height = size * size / estimateTypeSize(l.text, size); // box = size × reach
    return l;
}

const std::string kBody = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor.";
const std::string kArabicBody = "الحمد لله رب العالمين والصلاة والسلام على رسوله الكريم وعلى آله وصحبه أجمعين";

std::vector<TextLine> book() {
    std::vector<TextLine> lines;
    for (int page = 0; page < 6; ++page) {
        double y = 30;
        lines.push_back(line(page, "Running header of the book", 9, 24, 110)); // repeated in the margin
        lines.push_back(line(page, "THE BOOK TITLE", 14, 10, 120));          // large, identical on every page
        if (page % 2 == 0) {
            lines.push_back(line(page, "Chapter " + std::to_string(page / 2 + 1) + ": The Beginning", 20, y += 30, 240));
            lines.push_back(line(page, "and Its Second Line", 20, y += 22, 180)); // same heading, next line
        }
        for (int i = 0; i < 8; ++i) lines.push_back(line(page, kBody, 11, y += 15));
        lines.push_back(line(page, "Section " + std::to_string(page + 1) + ".1 Methods", 15, y += 26, 150));
        for (int i = 0; i < 8; ++i) lines.push_back(line(page, kBody, 11, y += 15));
        lines.push_back(line(page, "Chapter summaries follow in the next part of this long sentence here.", 11, y += 15));
        lines.push_back(line(page, "Exercises", 11, y += 20, 60)); // body size, short, not heading-like: ignored
        lines.push_back(line(page, "الفصل الثاني", 11, y += 20, 70)); // body size, starts like a heading
        lines.push_back(line(page, kArabicBody, 11, y += 15));
        lines.push_back(line(page, std::to_string(page + 1), 9, 800, 6)); // page number
    }
    return lines;
}

void checkEstimates() {
    // Real Qt PDF boxes (Helvetica): 11 pt body with descenders, 15 pt title without.
    require(std::abs(estimateTypeSize("Lorem ipsum dolor sit amet, consectetur adipiscing", 10.31) - 11) < 0.6,
            "Body size estimated from a line with descenders.");
    require(std::abs(estimateTypeSize("Section 1.1 Methods", 11.33) - 15) < 1.0,
            "Heading size estimated from a line without descenders.");
    require(std::abs(estimateTypeSize("Chapter 1: The Beginning", 19.06) - 20) < 1.0, "Large heading size.");
    require(estimateTypeSize("x", 0) == 0 && estimateTypeSize("x", std::nan("")) == 0, "Bad boxes give no size.");
    require(startsLikeHeading("Chapter 3") && startsLikeHeading("CHAPTER ONE") && startsLikeHeading("الفصل الأول") &&
                startsLikeHeading("باب ۲") && startsLikeHeading("حصہ اول") && startsLikeHeading("Part II: Methods"),
            "Heading words in English, Arabic and Urdu.");
    require(!startsLikeHeading("Chapters are long") && !startsLikeHeading("Partial results") &&
                !startsLikeHeading("بابل") && !startsLikeHeading("The chapter"),
            "Only whole heading words at the start count.");
}

void checkDetection() {
    const auto lines = book();
    const auto result = detectHeadings(lines);
    require(std::abs(result.bodySize - 11) <= 0.5, "Body size is the size carrying the most text.");
    std::vector<std::string> titles;
    for (const auto& h : result.headings) titles.push_back(h.title);
    // 3 chapters (two lines joined), 6 sections, 6 Arabic heading-like lines.
    require(result.headings.size() == 15, "Expected 15 headings, got " + std::to_string(result.headings.size()));
    const auto& first = result.headings[0];
    require(first.title == "Chapter 1: The Beginning and Its Second Line" && first.level == 0 && first.page == 0,
            "Two-line chapter heading joined at level 0: " + first.title);
    require(result.headings[1].title == "Section 1.1 Methods" && result.headings[1].level == 1, "Sections are level 1.");
    require(result.headings[2].title == "الفصل الثاني" && result.headings[2].level == 2,
            "Heading-like body-size lines come last.");
    require(first.fy > 0 && first.fy < 0.1 && first.fx > 0.1 && first.fx < 0.2, "Destination near the heading's top-left.");
    for (const auto& h : result.headings) {
        require(h.title.find("Running header") == std::string::npos && h.title != "THE BOOK TITLE",
                "Running headers are left out, small or large.");
        require(h.title != "Exercises" && h.title.size() > 2, "Body-size lines and page numbers are left out.");
    }
    HeadingOptions two;
    two.maxLevels = 2;
    require(detectHeadings(lines, two).headings.size() == 9, "Two levels keep chapters and sections only.");
    HeadingOptions capped;
    capped.maxHeadings = 4;
    const auto limited = detectHeadings(lines, capped);
    require(limited.headings.size() == 4 && limited.truncated, "The preview limit is reported.");
    require(detectHeadings({}).headings.empty() && detectHeadings({}).bodySize == 0, "No text, no headings.");
}

} // namespace

int main() {
    try {
        checkEstimates();
        checkDetection();
    } catch (const std::exception& error) {
        std::cerr << "Heading detector test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Heading detector passed.\n";
    return 0;
}
