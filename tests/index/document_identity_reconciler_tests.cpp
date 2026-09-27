#include "core/index/DocumentIdentityReconciler.h"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

using namespace atlas::index;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

void checkSameLocationAndReplacement() {
    auto result = reconcileDocumentIdentity({
        .samePath = true,
        .sameFilesystemIdentity = true,
        .sameRevision = true,
    });
    require(result.decision == IdentityDecision::sameLocationUnchanged && !result.mayRelinkAutomatically,
        "The same path, filesystem identity and revision must remain the same observation.");

    result = reconcileDocumentIdentity({
        .samePath = true,
        .sameFilesystemIdentity = true,
        .sameRevision = false,
    });
    require(result.decision == IdentityDecision::sameLocationChanged,
        "A changed revision with stable filesystem identity must be explicit.");

    result = reconcileDocumentIdentity({
        .samePath = true,
        .sameFilesystemIdentity = false,
        .sameContentFingerprint = true,
    });
    require(result.decision == IdentityDecision::ambiguous,
        "A different filesystem object at the same path must not steal the preceding book identity.");
}

void checkMoveCopyAndOfflineRules() {
    auto result = reconcileDocumentIdentity({
        .previousLocation = PreviousLocationState::missingAfterCompleteScan,
        .sameFilesystemIdentity = true,
        .sameRevision = true,
    });
    require(result.decision == IdentityDecision::confidentMove && result.mayRelinkAutomatically,
        "A missing old path and preserved filesystem identity may prove a rename or move.");

    result = reconcileDocumentIdentity({
        .previousLocation = PreviousLocationState::available,
        .sameFilesystemIdentity = false,
        .samePdfIdentifier = true,
        .sameContentFingerprint = true,
    });
    require(result.decision == IdentityDecision::distinctCopy && !result.mayRelinkAutomatically,
        "A surviving original and byte-matching candidate must remain distinct copies.");

    result = reconcileDocumentIdentity({
        .previousLocation = PreviousLocationState::missingAfterCompleteScan,
        .sameFilesystemIdentity = false,
        .samePdfIdentifier = true,
        .sameContentFingerprint = true,
    });
    require(result.decision == IdentityDecision::ambiguous && !result.mayRelinkAutomatically,
        "Matching content without filesystem identity cannot distinguish a move from copy-then-delete.");

    result = reconcileDocumentIdentity({
        .previousLocation = PreviousLocationState::rootOffline,
        .sameFilesystemIdentity = true,
        .sameContentFingerprint = true,
    });
    require(result.decision == IdentityDecision::ambiguous && !result.mayRelinkAutomatically,
        "An offline source can never authorize automatic reassociation.");

    result = reconcileDocumentIdentity({
        .previousLocation = PreviousLocationState::rootOffline,
        .sameFilesystemIdentity = false,
        .samePdfIdentifier = false,
        .sameContentFingerprint = false,
    });
    require(result.decision == IdentityDecision::ambiguous && !result.mayRelinkAutomatically,
        "An offline source must remain unknown even when a candidate has no matching evidence.");

    result = reconcileDocumentIdentity({
        .previousLocation = PreviousLocationState::scanIncomplete,
        .sameFilesystemIdentity = false,
        .samePdfIdentifier = false,
        .sameContentFingerprint = false,
    });
    require(result.decision == IdentityDecision::ambiguous && !result.mayRelinkAutomatically,
        "An incomplete scan must never prove that a candidate is unrelated.");

    result = reconcileDocumentIdentity({
        .previousLocation = PreviousLocationState::missingAfterCompleteScan,
        .sameFilesystemIdentity = false,
        .samePdfIdentifier = false,
        .sameContentFingerprint = false,
    });
    require(result.decision == IdentityDecision::newDocument,
        "A candidate with no matching evidence must remain a new document.");
}

} // namespace

int main() {
    try {
        checkSameLocationAndReplacement();
        checkMoveCopyAndOfflineRules();
    } catch (const std::exception& error) {
        std::cerr << "Document identity reconciliation test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Document identity same-path, change, move, copy, ambiguity and offline rules passed.\n";
}
