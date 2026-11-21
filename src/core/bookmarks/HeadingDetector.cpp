#include "core/bookmarks/HeadingDetector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>
#include <string_view>
#include <unordered_map>

namespace atlas::bookmarks {
namespace {

// Decodes one code point; invalid bytes come back as U+FFFD, one byte at a time.
char32_t next(std::string_view text, std::size_t& i) {
    const auto c = static_cast<unsigned char>(text[i]);
    std::size_t length = 1;
    char32_t cp = 0xFFFD;
    if (c < 0x80) cp = c;
    else if (c >= 0xC2 && c < 0xE0) { cp = c & 0x1Fu; length = 2; }
    else if (c >= 0xE0 && c < 0xF0) { cp = c & 0x0Fu; length = 3; }
    else if (c >= 0xF0 && c < 0xF5) { cp = c & 0x07u; length = 4; }
    if (length > 1) {
        if (i + length > text.size()) { ++i; return 0xFFFD; }
        for (std::size_t k = 1; k < length; ++k) {
            const auto b = static_cast<unsigned char>(text[i + k]);
            if ((b & 0xC0u) != 0x80u) { ++i; return 0xFFFD; }
            cp = (cp << 6) | (b & 0x3Fu);
        }
    }
    i += length;
    return cp;
}

bool isArabicScript(char32_t c) {
    return (c >= 0x0600 && c <= 0x06FF) || (c >= 0x0750 && c <= 0x077F) || (c >= 0x08A0 && c <= 0x08FF) ||
           (c >= 0xFB50 && c <= 0xFDFF) || (c >= 0xFE70 && c <= 0xFEFF);
}

bool isDigit(char32_t c) {
    return (c >= '0' && c <= '9') || (c >= 0x0660 && c <= 0x0669) || (c >= 0x06F0 && c <= 0x06F9);
}

bool isLetter(char32_t c) {
    if (c < 0x80) return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    if (isDigit(c) || c == 0xFFFD) return false;
    if (c >= 0x2000 && c <= 0x2BFF) return false; // punctuation, symbols, arrows
    if (c >= 0x064B && c <= 0x065F) return false; // harakat
    if (c == 0x0640 || c == 0x060C || c == 0x061B || c == 0x061F || c == 0x06D4) return false;
    return true;
}

// Vertical reach of a line as fractions of the em: how far its tallest glyph
// rises above the baseline and its lowest falls below.
struct Reach final {
    double up{};
    double down{};
};

Reach reachOf(std::string_view text) {
    Reach reach;
    for (std::size_t i = 0; i < text.size();) {
        const char32_t c = next(text, i);
        double up = 0;
        double down = 0;
        if (c < 0x80) {
            if (c == ' ') continue;
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || std::string_view{"bdfhiklt!?&%$#@\"'"}.find(static_cast<char>(c)) != std::string_view::npos) up = 0.72;
            else if (c >= 'a' && c <= 'z') up = 0.52;
            if (std::string_view{"gjpqy,;()[]{}|/_Q"}.find(static_cast<char>(c)) != std::string_view::npos) down = 0.21;
            if (std::string_view{"()[]{}|/"}.find(static_cast<char>(c)) != std::string_view::npos) up = 0.75;
        } else if (isArabicScript(c)) {
            if (c >= 0x064B && c <= 0x065F) continue; // marks sit on letters
            static constexpr std::array<char32_t, 14> tall{0x0622, 0x0623, 0x0625, 0x0627, 0x0671, 0x0644, 0x0643,
                                                         0x06A9, 0x06AF, 0x0637, 0x0638, 0x0624, 0x0626, 0x0670};
            static constexpr std::array<char32_t, 26> low{0x0631, 0x0632, 0x0648, 0x064A, 0x0649, 0x06CC, 0x06D2,
                                                        0x0646, 0x0639, 0x063A, 0x062C, 0x062D, 0x062E, 0x0645,
                                                        0x0633, 0x0634, 0x0635, 0x0636, 0x0642, 0x0628, 0x0691,
                                                        0x0698, 0x0686, 0x06BA, 0x06C1, 0x0626};
            up = std::find(tall.begin(), tall.end(), c) != tall.end() ? 0.80 : 0.50;
            if (std::find(low.begin(), low.end(), c) != low.end()) down = 0.35;
        } else if (isDigit(c)) {
            up = 0.72;
        } else if (isLetter(c)) {
            up = 0.75; // other scripts: a typical cap/ascender height
            down = 0.12;
        }
        reach.up = std::max(reach.up, up);
        reach.down = std::max(reach.down, down);
    }
    return reach;
}

std::string simplified(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    bool space = false;
    for (const char ch : text) {
        const bool isSpace = ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v';
        if (isSpace) {
            space = !out.empty();
            continue;
        }
        if (space) out.push_back(' ');
        space = false;
        out.push_back(ch);
    }
    return out;
}

std::size_t letterCount(std::string_view text) {
    std::size_t n = 0;
    for (std::size_t i = 0; i < text.size();) n += isLetter(next(text, i)) ? 1 : 0;
    return n;
}

std::size_t charCount(std::string_view text) {
    std::size_t n = 0;
    for (std::size_t i = 0; i < text.size();) {
        if (next(text, i) != ' ') ++n;
    }
    return n;
}

std::size_t wordCount(std::string_view text) {
    std::size_t n = 0;
    bool in = false;
    for (const char ch : text) {
        if (ch == ' ') in = false;
        else if (!in) { in = true; ++n; }
    }
    return n;
}

// Running headers and footers: the same text in the top or bottom tenth of
// many pages. Small type is compared without digits (page numbers); large
// type keeps them, because chapter openings near the top ("Chapter 1",
// "Chapter 2") differ only there.
std::string marginKey(std::string_view text, double size, bool keepDigits) {
    std::string key;
    for (std::size_t i = 0; i < text.size();) {
        const std::size_t start = i;
        const char32_t c = next(text, i);
        if ((!keepDigits && isDigit(c)) || c == ' ') continue;
        if (c >= 'A' && c <= 'Z') key.push_back(static_cast<char>(c - 'A' + 'a'));
        else key.append(text.substr(start, i - start));
    }
    return key + '#' + std::to_string(static_cast<int>(std::lround(size)));
}

std::string lowerAscii(std::string_view text) {
    std::string out{text};
    for (char& ch : out) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    }
    return out;
}

} // namespace

double estimateTypeSize(std::string_view text, double boxHeight) {
    if (!(boxHeight > 0) || !std::isfinite(boxHeight)) return 0;
    const Reach reach = reachOf(text);
    const double span = reach.up + reach.down;
    return span > 0.2 ? boxHeight / span : boxHeight;
}

bool startsLikeHeading(std::string_view raw) {
    const std::string text = lowerAscii(simplified(raw));
    static constexpr std::array<std::string_view, 54> words{
        // English
        "chapter", "part", "section", "book", "appendix", "preface", "introduction", "conclusion", "contents",
        "foreword", "prologue", "epilogue", "lesson", "unit", "bibliography", "index",
        // Arabic
        "الباب", "باب", "الفصل", "فصل", "المبحث", "مبحث", "المطلب", "مطلب", "الكتاب", "كتاب", "المقدمة",
        "مقدمة", "الخاتمة", "خاتمة", "الجزء", "القسم", "قسم", "التمهيد", "تمهيد", "الفهرس", "فهرس", "المسألة",
        "مسألة", "الفرع", "فرع", "الدرس", "درس",
        // Urdu
        "حصہ", "مقدمہ", "پیش لفظ", "دیباچہ", "سبق", "اختتامیہ", "فہرست", "تمہید", "عرض مؤلف", "ابتدائیہ", "پیش گفتار"};
    for (const auto word : words) {
        if (text.size() < word.size() || text.compare(0, word.size(), word) != 0) continue;
        if (text.size() == word.size()) return true;
        const auto after = static_cast<unsigned char>(text[word.size()]);
        // A whole word: followed by a space, digit or punctuation, not more letters.
        if (after == ' ' || after == ':' || after == '.' || after == '-' || (after >= '0' && after <= '9')) return true;
    }
    return false;
}

HeadingResult detectHeadings(const std::vector<TextLine>& lines, const HeadingOptions& options) {
    HeadingResult result;
    result.linesRead = lines.size();
    const int maxLevels = std::clamp(options.maxLevels, 1, 4);

    struct Line final {
        const TextLine* source{};
        std::string text;
        double size{};
        bool margin{};
    };
    std::vector<Line> usable;
    usable.reserve(lines.size());
    std::set<int> pagesWithText;
    for (const auto& line : lines) {
        std::string text = simplified(line.text);
        if (text.empty() || !(line.height > 0) || !(line.pageHeight > 0) || !(line.pageWidth > 0)) continue;
        const double size = estimateTypeSize(text, line.height);
        if (!(size > 0)) continue;
        const double middle = (line.top + line.height / 2) / line.pageHeight;
        usable.push_back({&line, std::move(text), size, middle < 0.1 || middle > 0.9});
        pagesWithText.insert(line.page);
    }
    if (usable.empty()) return result;

    // Body size: the size carrying the most characters (half-point buckets).
    std::map<int, std::size_t> weight;
    for (const auto& line : usable) weight[static_cast<int>(std::lround(line.size * 2))] += charCount(line.text);
    const auto body = std::max_element(weight.begin(), weight.end(),
                                       [](const auto& a, const auto& b) { return a.second < b.second; });
    result.bodySize = body->first / 2.0;
    const double bodySize = result.bodySize;

    // Typical width of a full body line, for "short line" checks.
    std::vector<double> bodyWidths;
    for (const auto& line : usable) {
        if (std::abs(line.size - bodySize) <= bodySize * 0.08 && charCount(line.text) >= 30) bodyWidths.push_back(line.source->width);
    }
    double bodyWidth = 0;
    if (!bodyWidths.empty()) {
        std::nth_element(bodyWidths.begin(), bodyWidths.begin() + static_cast<std::ptrdiff_t>(bodyWidths.size() / 2), bodyWidths.end());
        bodyWidth = bodyWidths[bodyWidths.size() / 2];
    }

    // Running headers and footers.
    std::unordered_map<std::string, std::set<int>> marginPages;
    for (const auto& line : usable) {
        if (line.margin) marginPages[marginKey(line.text, line.size, line.size >= bodySize * options.largerBy)].insert(line.source->page);
    }
    const auto repeated = [&](const Line& line) {
        if (!line.margin) return false;
        const auto found = marginPages.find(marginKey(line.text, line.size, line.size >= bodySize * options.largerBy));
        const auto pages = found == marginPages.end() ? 0 : found->second.size();
        return pages >= 3 && static_cast<double>(pages) >= 0.2 * static_cast<double>(pagesWithText.size());
    };

    struct Candidate final {
        const Line* line{};
        bool bySize{};
    };
    std::vector<Candidate> candidates;
    for (const auto& line : usable) {
        if (line.text.size() > options.maxTitleBytes || letterCount(line.text) < 2 || repeated(line)) continue;
        if (line.size >= bodySize * options.largerBy) {
            candidates.push_back({&line, true});
        } else if (line.size >= bodySize * 0.92 && wordCount(line.text) <= 10 && startsLikeHeading(line.text) &&
                   (bodyWidth <= 0 || line.source->width <= bodyWidth * 0.7)) {
            candidates.push_back({&line, false});
        }
    }

    // Levels: sizes within 10 % of each other are one level, largest first;
    // heading-like lines at body size come after every size level.
    std::vector<double> sizes;
    for (const auto& candidate : candidates) {
        if (candidate.bySize) sizes.push_back(candidate.line->size);
    }
    std::sort(sizes.begin(), sizes.end(), std::greater<>());
    std::vector<double> clusterTops;
    for (const double size : sizes) {
        if (clusterTops.empty() || size < clusterTops.back() * 0.9) clusterTops.push_back(size);
    }
    const auto levelOf = [&](const Candidate& candidate) {
        if (!candidate.bySize) return static_cast<int>(clusterTops.size());
        int level = 0;
        for (std::size_t i = 0; i < clusterTops.size(); ++i) {
            if (candidate.line->size >= clusterTops[i] * 0.9 - 1e-9) {
                level = static_cast<int>(i);
                break;
            }
        }
        return level;
    };

    for (const auto& candidate : candidates) {
        const int level = levelOf(candidate);
        if (level >= maxLevels) continue;
        const TextLine& source = *candidate.line->source;
        DetectedHeading heading;
        heading.page = source.page;
        heading.title = candidate.line->text;
        heading.level = level;
        heading.size = candidate.line->size;
        heading.fx = std::clamp(source.left / source.pageWidth, 0.0, 1.0);
        heading.fy = std::clamp((source.top - candidate.line->size * 0.25) / source.pageHeight, 0.0, 1.0);
        result.headings.push_back(std::move(heading));
    }

    // Join continuation lines: same page and level, directly below.
    std::vector<DetectedHeading> joined;
    joined.reserve(result.headings.size());
    std::vector<double> bottoms; // bottom of the last line joined into each heading, in points
    std::vector<int> partCount;
    {
        std::size_t index = 0;
        for (const auto& candidate : candidates) {
            const int level = levelOf(candidate);
            if (level >= maxLevels) continue;
            auto& heading = result.headings[index++];
            const TextLine& source = *candidate.line->source;
            if (!joined.empty()) {
                auto& last = joined.back();
                const double gap = source.top - bottoms.back();
                if (last.page == heading.page && last.level == heading.level && partCount.back() < 3 &&
                    gap >= -0.1 * heading.size && gap <= 0.8 * heading.size &&
                    last.title.size() + heading.title.size() < options.maxTitleBytes) {
                    last.title += ' ';
                    last.title += heading.title;
                    bottoms.back() = source.top + source.height;
                    ++partCount.back();
                    continue;
                }
            }
            bottoms.push_back(source.top + source.height);
            partCount.push_back(1);
            joined.push_back(std::move(heading));
        }
    }
    if (joined.size() > options.maxHeadings) {
        joined.resize(options.maxHeadings);
        result.truncated = true;
    }
    result.headings = std::move(joined);
    return result;
}

} // namespace atlas::bookmarks
