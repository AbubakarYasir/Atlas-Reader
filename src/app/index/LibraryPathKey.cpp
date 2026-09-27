#include "app/index/LibraryPathKey.h"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace atlas::index {

std::string libraryPathKey(const std::filesystem::path& path) {
    std::error_code error;
    auto normalized = std::filesystem::absolute(path, error);
    if (error) normalized = path;
    const auto utf8 = normalized.lexically_normal().generic_u8string();
    std::string key{reinterpret_cast<const char*>(utf8.data()), utf8.size()};
#ifdef _WIN32
    // Windows paths are case-insensitive for Atlas's supported local roots.
    // ASCII folding covers drive letters and ordinary Latin path aliases while
    // preserving Arabic, Urdu and other non-cased scripts byte-for-byte.
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
#endif
    return key;
}

} // namespace atlas::index
