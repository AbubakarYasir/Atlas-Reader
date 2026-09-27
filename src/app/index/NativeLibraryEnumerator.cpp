#include "app/index/NativeLibraryEnumerator.h"

#include <system_error>

namespace atlas::index {

DirectoryEnumeration NativeLibraryEnumerator::forEachEntry(
    const std::filesystem::path& directory,
    const std::function<bool(const EnumeratedEntry&)>& visitor) const {
    std::error_code error;
    const auto rootStatus = std::filesystem::symlink_status(directory, error);
    if (error || std::filesystem::is_symlink(rootStatus) || !std::filesystem::is_directory(rootStatus)) {
        return {false, false};
    }
    error.clear();
    std::filesystem::directory_iterator current{directory, std::filesystem::directory_options::none, error};
    if (error) return {false, false};

    const std::filesystem::directory_iterator end;
    while (current != end) {
        const auto path = current->path();
        const auto status = current->symlink_status(error);
        if (error) {
            error.clear();
            if (!visitor({path, EnumeratedEntryKind::inaccessible})) return {true, false};
        } else {
            EnumeratedEntryKind kind = EnumeratedEntryKind::other;
            if (std::filesystem::is_symlink(status)) kind = EnumeratedEntryKind::symlink;
            else if (std::filesystem::is_directory(status)) kind = EnumeratedEntryKind::directory;
            else if (std::filesystem::is_regular_file(status)) kind = EnumeratedEntryKind::regularFile;
            if (!visitor({path, kind})) return {true, false};
        }

        current.increment(error);
        if (error) return {true, false};
    }
    return {true, true};
}

} // namespace atlas::index
