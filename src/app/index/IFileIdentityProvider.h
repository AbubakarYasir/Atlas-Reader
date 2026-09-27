#pragma once

#include <filesystem>
#include <string>

namespace atlas::index {

enum class FileIdentityState {
    available,
    missing,
    inaccessible,
    unsupported,
};

struct FileIdentityResult final {
    FileIdentityState state{FileIdentityState::unsupported};
    std::string identity;
};

class IFileIdentityProvider {
public:
    virtual ~IFileIdentityProvider() = default;

    [[nodiscard]] virtual FileIdentityResult inspect(
        const std::filesystem::path& source) const = 0;
};

} // namespace atlas::index

