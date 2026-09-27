#include "core/index/DocumentIdentityReconciler.h"

namespace atlas::index {
namespace {

[[nodiscard]] bool anySimilarity(const DocumentIdentitySignals& evidence) {
    return evidence.sameFilesystemIdentity.value_or(false)
        || evidence.sameRevision.value_or(false)
        || evidence.samePdfIdentifier.value_or(false)
        || evidence.sameContentFingerprint.value_or(false);
}

} // namespace

DocumentIdentityResolution reconcileDocumentIdentity(const DocumentIdentitySignals& evidence) {
    if (evidence.samePath) {
        if (evidence.sameFilesystemIdentity == false) {
            return {IdentityDecision::ambiguous, false};
        }
        if (evidence.sameFilesystemIdentity == true) {
            return {
                evidence.sameRevision == true
                    ? IdentityDecision::sameLocationUnchanged
                    : IdentityDecision::sameLocationChanged,
                false,
            };
        }
        return {IdentityDecision::ambiguous, false};
    }

    if (evidence.previousLocation == PreviousLocationState::available) {
        return {IdentityDecision::distinctCopy, false};
    }
    if (evidence.previousLocation == PreviousLocationState::rootOffline
        || evidence.previousLocation == PreviousLocationState::scanIncomplete) {
        return {IdentityDecision::ambiguous, false};
    }
    if (evidence.sameFilesystemIdentity == true) {
        return {IdentityDecision::confidentMove, true};
    }
    return {anySimilarity(evidence) ? IdentityDecision::ambiguous : IdentityDecision::newDocument, false};
}

} // namespace atlas::index
