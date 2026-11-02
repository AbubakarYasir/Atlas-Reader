#pragma once

// Text search over extracted page text (N4.5). Pure C++ (UTF-16 code units,
// as PDF engines return them). Matching normalizes case for Latin and, for
// Arabic and Urdu, ignores diacritics, tatweel and joiners and unifies common
// letter variants (alef forms, yeh forms, kaf/keheh, teh marbuta/heh, heh
// forms). Results are reported in indices of the original text so the engine
// can turn them into highlight rectangles. Nothing is ever changed in the PDF.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace atlas::reader {

struct SearchOptions final {
    bool matchCase{};
    bool wholeWords{};
};

struct TextMatch final {
    std::size_t start{};  // index into the original text
    std::size_t length{}; // code units in the original text

    friend bool operator==(const TextMatch&, const TextMatch&) = default;
};

struct NormalizedText final {
    std::u16string text;
    std::vector<std::size_t> source; // source[i] = original index of text[i]
};

[[nodiscard]] NormalizedText normalizeForSearch(std::u16string_view text, const SearchOptions& options);
[[nodiscard]] std::vector<TextMatch> findMatches(std::u16string_view haystack, std::u16string_view query,
                                                 const SearchOptions& options);
[[nodiscard]] bool isWordCharacter(char16_t c) noexcept;
// Words in extracted text (for Word Count): runs of word characters.
[[nodiscard]] std::size_t countWords(std::u16string_view text) noexcept;

} // namespace atlas::reader
