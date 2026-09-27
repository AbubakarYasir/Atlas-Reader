#pragma once

#include "app/index/ILibraryEnumerator.h"

namespace atlas::index {

class NativeLibraryEnumerator final : public ILibraryEnumerator {
public:
    [[nodiscard]] DirectoryEnumeration forEachEntry(
        const std::filesystem::path& directory,
        const std::function<bool(const EnumeratedEntry&)>& visitor) const override;
};

} // namespace atlas::index
