#pragma once

#include "app/index/IFileIdentityProvider.h"

namespace atlas::index {

class NativeFileIdentityProvider final : public IFileIdentityProvider {
public:
    [[nodiscard]] FileIdentityResult inspect(
        const std::filesystem::path& source) const override;
};

} // namespace atlas::index

