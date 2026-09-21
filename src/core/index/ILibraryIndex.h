#pragma once

#include "core/document/DocumentCapabilities.h"

#include <filesystem>
#include <string>
#include <vector>

namespace atlas::index {

struct LibraryRecord final {
    std::string id;
    std::filesystem::path source;
    std::string title;
    std::string author;
    document::Availability availability{document::Availability::available};
};

class ILibraryIndex {
public:
    virtual ~ILibraryIndex() = default;

    [[nodiscard]] virtual std::vector<LibraryRecord> search(std::string query) const = 0;
};

} // namespace atlas::index
