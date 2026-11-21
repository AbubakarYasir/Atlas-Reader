#include "core/document/CapabilityPolicy.h"

namespace atlas::document {

CapabilityAssessment assessCapabilities(const FileFacts& file, const PdfFacts& pdf,
                                        const RevisionFacts& revision) {
    CapabilityAssessment result;
    auto& caps = result.capabilities;
    auto& reasons = result.reasons;

    // ---- can it be read at all?
    if (!file.exists) {
        caps = {Availability::missing, MutationSafety::unsupported, false, false, false, false, false};
        reasons.push_back(CapabilityReason::missing);
        return result; // research stays, never purged
    }
    if (file.offline || file.cloudPlaceholder) {
        caps = {file.offline ? Availability::offline : Availability::cloudPlaceholder, MutationSafety::unsupported,
                false, false, false, false, false};
        reasons.push_back(file.offline ? CapabilityReason::offline : CapabilityReason::cloudPlaceholder);
        return result;
    }
    if (pdf.encryption == Encryption::unsupportedHandler) {
        caps = {Availability::unsupported, MutationSafety::unsupported, false, false, false, false, false};
        reasons.push_back(CapabilityReason::unsupportedSecurity);
        return result;
    }
    if (pdf.encryption == Encryption::openPasswordRequired) {
        caps = {Availability::locked, MutationSafety::unsupported, false, false, false, false, false};
        reasons.push_back(CapabilityReason::passwordRequired);
        return result;
    }
    if (!pdf.readable) {
        caps = {Availability::unreadable, MutationSafety::unsupported, false, false, false, false, false};
        reasons.push_back(CapabilityReason::unreadable);
        return result;
    }

    caps.availability = Availability::available;
    caps.canRead = true;
    caps.canRender = true;
    caps.canSearchText = pdf.permissions.extract || pdf.encryption != Encryption::openedWithUserPassword;

    // ---- may the PDF itself be changed?
    // Owner password (or no encryption) means the PDF's permission flags do not apply.
    const bool restricted = pdf.encryption == Encryption::openedWithUserPassword;
    const bool outlinePermitted = !restricted || pdf.permissions.modifyContents || pdf.permissions.assemble;
    const bool annotationsPermitted = !restricted || pdf.permissions.modifyAnnotations;

    // Anything that blocks every write; the first one names the mutation state.
    MutationSafety blocked = MutationSafety::writable;
    auto block = [&](MutationSafety state, CapabilityReason reason) {
        if (blocked == MutationSafety::writable) blocked = state;
        reasons.push_back(reason);
    };
    if (revision.changedExternally) block(MutationSafety::externalConflict, CapabilityReason::changedExternally);
    if (!pdf.inspected) block(MutationSafety::unsupported, CapabilityReason::notInspected);
    if (pdf.malformed) block(MutationSafety::unsupported, CapabilityReason::malformed);
    if (pdf.certified) block(MutationSafety::signedOrCertified, CapabilityReason::certified);
    else if (pdf.hasSignatures) block(MutationSafety::signedOrCertified, CapabilityReason::signatures);
    if (file.readOnlyAttribute) block(MutationSafety::filesystemReadOnly, CapabilityReason::fileReadOnly);
    if (!file.folderWritable) block(MutationSafety::filesystemReadOnly, CapabilityReason::folderReadOnly);
    if (file.sharingLocked) block(MutationSafety::sharingLocked, CapabilityReason::sharingLocked);

    // The PDF's own permissions may allow one kind of change and not the other.
    if (!outlinePermitted) reasons.push_back(CapabilityReason::outlineNotPermitted);
    if (!annotationsPermitted) reasons.push_back(CapabilityReason::annotationsNotPermitted);

    if (blocked != MutationSafety::writable) {
        caps.mutation = blocked;
        caps.canEmbedOutline = false;
        caps.canEmbedAnnotations = false;
    } else {
        caps.canEmbedOutline = outlinePermitted;
        caps.canEmbedAnnotations = annotationsPermitted;
        caps.mutation = outlinePermitted && annotationsPermitted ? MutationSafety::writable
                                                                 : MutationSafety::permissionRestricted;
    }
    if (caps.mutation == MutationSafety::filesystemReadOnly) caps.availability = Availability::readOnly;
    return result;
}

std::string reasonName(CapabilityReason reason) {
    switch (reason) {
    case CapabilityReason::missing: return "missing";
    case CapabilityReason::offline: return "offline";
    case CapabilityReason::cloudPlaceholder: return "cloudPlaceholder";
    case CapabilityReason::passwordRequired: return "passwordRequired";
    case CapabilityReason::unsupportedSecurity: return "unsupportedSecurity";
    case CapabilityReason::unreadable: return "unreadable";
    case CapabilityReason::malformed: return "malformed";
    case CapabilityReason::notInspected: return "notInspected";
    case CapabilityReason::outlineNotPermitted: return "outlineNotPermitted";
    case CapabilityReason::annotationsNotPermitted: return "annotationsNotPermitted";
    case CapabilityReason::fileReadOnly: return "fileReadOnly";
    case CapabilityReason::folderReadOnly: return "folderReadOnly";
    case CapabilityReason::sharingLocked: return "sharingLocked";
    case CapabilityReason::signatures: return "signatures";
    case CapabilityReason::certified: return "certified";
    case CapabilityReason::changedExternally: return "changedExternally";
    }
    return "unknown";
}

} // namespace atlas::document
