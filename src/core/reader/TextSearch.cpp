#include "core/reader/TextSearch.h"

#include <algorithm>

namespace atlas::reader {

namespace {

bool ignorable(char16_t c) noexcept {
    return (c >= 0x064B && c <= 0x065F) // Arabic harakat
           || c == 0x0670                // superscript alef
           || (c >= 0x06D6 && c <= 0x06ED) // Quranic annotation marks
           || c == 0x0640                // tatweel
           || c == 0x200C || c == 0x200D // zero-width (non-)joiner
           || c == 0x00AD;               // soft hyphen
}

// Arabic Presentation Forms (many PDFs store shaped glyphs) → base letters.
// Returns the number of letters written to `out` (0: not a presentation form).
int unshape(char16_t c, char16_t out[2]) noexcept {
    if (c >= 0xFE70 && c <= 0xFE7F) return -1; // shaped harakat: ignorable
    if (c == 0xFE80) { out[0] = 0x0621; return 1; }
    if (c >= 0xFEF5 && c <= 0xFEFC) { // lam-alef ligatures
        static constexpr char16_t alefs[] = {0x0622, 0x0623, 0x0625, 0x0627};
        out[0] = 0x0644;
        out[1] = alefs[(c - 0xFEF5) / 2];
        return 2;
    }
    if (c >= 0xFE81 && c <= 0xFEF4) {
        // Each letter occupies 2 (isolated, final) or 4 (plus initial, medial) code points.
        struct Run { char16_t base; int count; };
        static constexpr Run runs[] = {
            {0x0622, 2}, {0x0623, 2}, {0x0624, 2}, {0x0625, 2}, {0x0626, 4}, {0x0627, 2}, {0x0628, 4},
            {0x0629, 2}, {0x062A, 4}, {0x062B, 4}, {0x062C, 4}, {0x062D, 4}, {0x062E, 4}, {0x062F, 2},
            {0x0630, 2}, {0x0631, 2}, {0x0632, 2}, {0x0633, 4}, {0x0634, 4}, {0x0635, 4}, {0x0636, 4},
            {0x0637, 4}, {0x0638, 4}, {0x0639, 4}, {0x063A, 4}, {0x0641, 4}, {0x0642, 4}, {0x0643, 4},
            {0x0644, 4}, {0x0645, 4}, {0x0646, 4}, {0x0647, 4}, {0x0648, 2}, {0x0649, 2}, {0x064A, 4}};
        int offset = c - 0xFE81;
        for (const auto& run : runs) {
            if (offset < run.count) { out[0] = run.base; return 1; }
            offset -= run.count;
        }
        return 0;
    }
    if (c >= 0xFB50 && c <= 0xFBFF) { // Presentation Forms-A: Urdu and Persian letters
        struct Range { char16_t first; char16_t last; char16_t base; };
        static constexpr Range ranges[] = {
            {0xFB56, 0xFB59, 0x067E}, {0xFB66, 0xFB69, 0x0679}, {0xFB7A, 0xFB7D, 0x0686},
            {0xFB84, 0xFB85, 0x0688}, {0xFB8A, 0xFB8B, 0x0698}, {0xFB8C, 0xFB8D, 0x0691},
            {0xFB8E, 0xFB91, 0x06A9}, {0xFB92, 0xFB95, 0x06AF}, {0xFB9E, 0xFB9F, 0x06BA},
            {0xFBA4, 0xFBA5, 0x06C0}, {0xFBA6, 0xFBA9, 0x06C1}, {0xFBAA, 0xFBAD, 0x06BE},
            {0xFBAE, 0xFBAF, 0x06D2}, {0xFBB0, 0xFBB1, 0x06D3}, {0xFBFC, 0xFBFF, 0x06CC}};
        for (const auto& range : ranges) {
            if (c >= range.first && c <= range.last) { out[0] = range.base; return 1; }
        }
    }
    return 0;
}

char16_t foldLetter(char16_t c, bool matchCase) noexcept {
    switch (c) {
    case 0x0622: case 0x0623: case 0x0625: case 0x0671: return 0x0627; // alef forms → alef
    case 0x0649: case 0x06CC: case 0x0626: return 0x064A;              // yeh forms → yeh
    case 0x0643: return 0x06A9;                                        // Arabic kaf → keheh
    case 0x0629: case 0x06C1: case 0x06BE: case 0x06D5: return 0x0647; // teh marbuta / heh forms → heh
    case 0x0624: return 0x0648;                                        // waw with hamza → waw
    default: break;
    }
    if (!matchCase) {
        if (c >= u'A' && c <= u'Z') return static_cast<char16_t>(c + 32);
        if ((c >= 0x00C0 && c <= 0x00DE) && c != 0x00D7) return static_cast<char16_t>(c + 32);
    }
    return c;
}

} // namespace

bool isWordCharacter(char16_t c) noexcept {
    if (c <= 0x7F) {
        return (c >= u'0' && c <= u'9') || (c >= u'A' && c <= u'Z') || (c >= u'a' && c <= u'z') || c == u'_';
    }
    switch (c) {
    case 0x00A0: case 0x00AB: case 0x00BB: case 0x060C: case 0x061B: case 0x061F: case 0x06D4:
    case 0x2013: case 0x2014: case 0x2018: case 0x2019: case 0x201C: case 0x201D: case 0x2026:
    case 0x3000: case 0x3001: case 0x3002: case 0xFD3E: case 0xFD3F:
        return false;
    default:
        return !(c >= 0x2000 && c <= 0x206F); // general punctuation and spaces
    }
}

NormalizedText normalizeForSearch(std::u16string_view text, const SearchOptions& options) {
    NormalizedText out;
    out.text.reserve(text.size());
    out.source.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char16_t c = text[i];
        if (ignorable(c)) continue;
        char16_t letters[2]{};
        const int count = unshape(c, letters);
        if (count < 0) continue;
        if (count == 0) {
            letters[0] = c;
        }
        for (int k = 0; k < std::max(count, 1); ++k) {
            out.text.push_back(foldLetter(letters[k], options.matchCase));
            out.source.push_back(i);
        }
    }
    return out;
}

std::vector<TextMatch> findMatches(std::u16string_view haystack, std::u16string_view query,
                                   const SearchOptions& options) {
    std::vector<TextMatch> matches;
    const NormalizedText hay = normalizeForSearch(haystack, options);
    const NormalizedText needle = normalizeForSearch(query, options);
    if (needle.text.empty() || hay.text.size() < needle.text.size()) return matches;
    std::size_t from = 0;
    while (from + needle.text.size() <= hay.text.size()) {
        const std::size_t at = hay.text.find(needle.text, from);
        if (at == std::u16string::npos) break;
        const std::size_t end = at + needle.text.size(); // exclusive, normalized
        bool accept = true;
        if (options.wholeWords) {
            const bool before = at > 0 && isWordCharacter(hay.text[at - 1]);
            const bool after = end < hay.text.size() && isWordCharacter(hay.text[end]);
            accept = !before && !after;
        }
        if (accept) {
            const std::size_t start = hay.source[at];
            // Include trailing ignorable marks (diacritics) of the last letter.
            std::size_t stop = hay.source[end - 1] + 1;
            while (stop < haystack.size() && ignorable(haystack[stop])) ++stop;
            matches.push_back({start, stop - start});
        }
        from = at + 1;
    }
    return matches;
}

std::size_t countWords(std::u16string_view text) noexcept {
    std::size_t words = 0;
    bool inWord = false;
    for (const char16_t c : text) {
        const bool word = isWordCharacter(c) && c != u' ' && c != u'\n' && c != u'\t' && c != u'\r';
        if (word && !inWord) ++words;
        inWord = word || (inWord && ignorable(c));
    }
    return words;
}

} // namespace atlas::reader
