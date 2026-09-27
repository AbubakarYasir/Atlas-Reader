#pragma once

#include <optional>

namespace atlas::index {

enum class PreviousLocationState {
    available,
    missingAfterCompleteScan,
    rootOffline,
    scanIncomplete,
};

enum class IdentityDecision {
    sameLocationUnchanged,
    sameLocationChanged,
    confidentMove,
    distinctCopy,
    newDocument,
    ambiguous,
};

struct DocumentIdentitySignals final {
    bool samePath{};
    PreviousLocationState previousLocation{PreviousLocationState::available};
    std::optional<bool> sameFilesystemIdentity;
    std::optional<bool> sameRevision;
    std::optional<bool> samePdfIdentifier;
    std::optional<bool> sameContentFingerprint;
};

struct DocumentIdentityResolution final {
    IdentityDecision decision{IdentityDecision::ambiguous};
    bool mayRelinkAutomatically{};
};

// Pure policy: evidence can propose a relationship, but only an unbroken
// filesystem identity may authorize automatic path relinking.
[[nodiscard]] DocumentIdentityResolution reconcileDocumentIdentity(
    const DocumentIdentitySignals& signals);

} // namespace atlas::index
