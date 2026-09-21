#pragma once

#include "core/document/DocumentCapabilities.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace atlas::pdf {

struct PageSize final {
    double widthPoints{};
    double heightPoints{};
};

class IPdfDocument {
public:
    virtual ~IPdfDocument() = default;

    [[nodiscard]] virtual std::size_t pageCount() const noexcept = 0;
    [[nodiscard]] virtual PageSize pageSize(std::size_t pageIndex) const = 0;
    [[nodiscard]] virtual document::DocumentCapabilities capabilities() const = 0;
};

class IPdfEngine {
public:
    virtual ~IPdfEngine() = default;

    [[nodiscard]] virtual std::unique_ptr<IPdfDocument> open(
        const std::filesystem::path& source) = 0;
};

} // namespace atlas::pdf
