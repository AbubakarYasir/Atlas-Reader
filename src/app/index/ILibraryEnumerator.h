#pragma once

#include <filesystem>
#include <functional>

namespace atlas::index {

enum class EnumeratedEntryKind { directory, regularFile, symlink, other, inaccessible };

struct EnumeratedEntry final {
    std::filesystem::path path;
    EnumeratedEntryKind kind{EnumeratedEntryKind::other};
};

struct DirectoryEnumeration final {
    bool opened{};
    bool complete{};
};

class ILibraryEnumerator {
public:
    virtual ~ILibraryEnumerator() = default;

    // Implementations stream entries and stop when visitor returns false.
    [[nodiscard]] virtual DirectoryEnumeration forEachEntry(
        const std::filesystem::path& directory,
        const std::function<bool(const EnumeratedEntry&)>& visitor) const = 0;
};

} // namespace atlas::index
