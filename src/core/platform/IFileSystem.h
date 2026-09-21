#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace atlas::platform {

struct FileRevision final {
    std::uintmax_t size{};
    std::uint64_t modifiedTicks{};
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    [[nodiscard]] virtual bool exists(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual bool isWritable(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual std::optional<FileRevision> revision(
        const std::filesystem::path& path) const = 0;
};

} // namespace atlas::platform
