#include "core/index/DocumentIdentityReconciler.h"

namespace atlas::index {
namespace {

[[nodiscard]] bool anySimilarity(const DocumentIdentitySignals& signals) {
    return signals.sameFilesystemIdentity.value_or(false)
        || signals.sameRevision.value_or(false)
        || signals.samePdfIdentifier.value_or(false)
        || signals.sameContentFingerprint.value_or(false);
}

} // namespace

DocumentIdentityResolution reconcileDocumentIdentity(const DocumentIdentitySignals& signals) {
    if (signals.samePath) {
        if (signals.sameFilesystemIdentity == false) {
            return {IdentityDecision::ambiguous, false};
        }
        if (signals.sameFilesystemIdentity == true) {
            return {
                signals.sameRevision == true
                    ? IdentityDecision::sameLocationUnchanged
                    : IdentityDecision::sameLocationChanged,
                false,
            };
        }
        return {IdentityDecision::ambiguous, false};
    }

    if (signals.previousLocation == PreviousLocationState::available) {
        return {IdentityDecision::distinctCopy, false};
    }
    if (signals.previousLocation == PreviousLocationState::rootOffline
        || signals.previousLocation == PreviousLocationState::scanIncomplete) {
        return {IdentityDecision::ambiguous, false};
    }
    if (signals.sameFilesystemIdentity == true) {
        return {IdentityDecision::confidentMove, true};
    }
    return {anySimilarity(signals) ? IdentityDecision::ambiguous : IdentityDecision::newDocument, false};
}

} // namespace atlas::index
