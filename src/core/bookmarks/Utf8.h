#pragma once

#include <string>
#include <string_view>

namespace atlas::bookmarks {

// Strict UTF-8 check (RFC 3629): rejects overlong forms, surrogates and code
// points above U+10FFFF. User text is stored exactly as given, so it must be
// valid UTF-8 to begin with; nothing is ever repaired, normalized or trimmed.
[[nodiscard]] constexpr bool isValidUtf8(std::string_view text) noexcept {
    std::size_t i = 0;
    const std::size_t n = text.size();
    while (i < n) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c < 0x80) {
            ++i;
            continue;
        }
        std::size_t length = 0;
        unsigned char low = 0x80;
        unsigned char high = 0xBF;
        if (c >= 0xC2 && c <= 0xDF) {
            length = 2;
        } else if (c >= 0xE0 && c <= 0xEF) {
            length = 3;
            if (c == 0xE0) low = 0xA0;  // no overlong
            if (c == 0xED) high = 0x9F; // no surrogates
        } else if (c >= 0xF0 && c <= 0xF4) {
            length = 4;
            if (c == 0xF0) low = 0x90;  // no overlong
            if (c == 0xF4) high = 0x8F; // ≤ U+10FFFF
        } else {
            return false;
        }
        if (n - i < length) return false;
        const auto second = static_cast<unsigned char>(text[i + 1]);
        if (second < low || second > high) return false;
        for (std::size_t k = 2; k < length; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            if (next < 0x80 || next > 0xBF) return false;
        }
        i += length;
    }
    return true;
}

// UTF-8 → UTF-16 for search. Input must be valid UTF-8 (isValidUtf8).
[[nodiscard]] inline std::u16string toUtf16(std::string_view text) {
    std::u16string out;
    out.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const auto c = static_cast<unsigned char>(text[i]);
        char32_t cp = 0;
        std::size_t length = 1;
        if (c < 0x80) cp = c;
        else if (c < 0xE0) { cp = c & 0x1Fu; length = 2; }
        else if (c < 0xF0) { cp = c & 0x0Fu; length = 3; }
        else { cp = c & 0x07u; length = 4; }
        if (i + length > text.size()) break;
        for (std::size_t k = 1; k < length; ++k) {
            cp = (cp << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3Fu);
        }
        if (cp >= 0x10000) {
            cp -= 0x10000;
            out.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
        } else {
            out.push_back(static_cast<char16_t>(cp));
        }
        i += length;
    }
    return out;
}

} // namespace atlas::bookmarks
