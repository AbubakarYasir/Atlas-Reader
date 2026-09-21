#pragma once

#include <cstdint>

namespace atlas::document {

enum class Availability : std::uint8_t {
    available,
    locked,
    readOnly,
    offline,
    missing,
    cloudPlaceholder,
    unreadable,
    unsupported,
};

enum class MutationSafety : std::uint8_t {
    writable,
    permissionRestricted,
    filesystemReadOnly,
    sharingLocked,
    signedOrCertified,
    externalConflict,
    unsupported,
};

struct DocumentCapabilities final {
    Availability availability{Availability::available};
    MutationSafety mutation{MutationSafety::writable};
    bool canRead{true};
    bool canSearchText{false};
    bool canRender{true};
    bool canEmbedOutline{false};
    bool canEmbedAnnotations{false};
};

} // namespace atlas::document
