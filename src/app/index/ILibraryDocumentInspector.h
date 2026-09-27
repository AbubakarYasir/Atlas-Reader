#pragma once

#include "core/document/DocumentCapabilities.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace atlas::index {

struct LibraryDocumentInspection final {
    std::filesystem::path source;
    std::string title;
    std::string author;
    document::Availability availability{document::Availability::unreadable};
    std::optional<std::uintmax_t> fileSizeBytes;
    std::optional<std::int64_t> modifiedUtcMs;
    std::optional<int> pageCount;
};

class ILibraryDocumentInspector {
public:
    virtual ~ILibraryDocumentInspector() = default;
    [[nodiscard]] virtual LibraryDocumentInspection inspect(
        const std::filesystem::path& source) const = 0;
};

} // namespace atlas::index
