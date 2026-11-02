#pragma once

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

} // namespace atlas::bookmarks
