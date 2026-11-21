// N5.1: every row of the capability table in docs/DATA_AND_SAFETY.md.
#include "core/document/CapabilityPolicy.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace atlas::document;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool has(const CapabilityAssessment& a, CapabilityReason r) {
    return std::find(a.reasons.begin(), a.reasons.end(), r) != a.reasons.end();
}

PdfFacts inspected() {
    PdfFacts pdf;
    pdf.inspected = true;
    return pdf;
}

PdfFacts userOpened(PdfPermissions permissions) {
    PdfFacts pdf = inspected();
    pdf.encryption = Encryption::openedWithUserPassword;
    pdf.permissions = permissions;
    return pdf;
}

} // namespace

int main() {
    try {
        const FileFacts normal;
        const RevisionFacts same;

        { // Normal writable
            const auto a = assessCapabilities(normal, inspected(), same);
            require(a.capabilities.canRead && a.capabilities.canEmbedOutline && a.capabilities.canEmbedAnnotations,
                    "A normal writable PDF can be read and embedded.");
            require(a.capabilities.mutation == MutationSafety::writable && a.reasons.empty(), "No reasons when writable.");
        }
        { // Before the structural check: never claim we can write.
            const auto a = assessCapabilities(normal, PdfFacts{}, same);
            require(a.capabilities.canRead && !a.capabilities.canEmbedOutline && has(a, CapabilityReason::notInspected),
                    "Uninspected documents read but never embed.");
        }
        { // Open password missing or wrong
            PdfFacts pdf = inspected();
            pdf.encryption = Encryption::openPasswordRequired;
            const auto a = assessCapabilities(normal, pdf, same);
            require(!a.capabilities.canRead && a.capabilities.availability == Availability::locked &&
                        a.canKeepLocalResearch && has(a, CapabilityReason::passwordRequired),
                    "A locked PDF cannot be read; research stays.");
        }
        { // Owner password: permission flags do not apply.
            PdfFacts pdf = userOpened({false, false, false, false, false, false});
            pdf.encryption = Encryption::openedWithOwnerPassword;
            const auto a = assessCapabilities(normal, pdf, same);
            require(a.capabilities.canEmbedOutline && a.capabilities.canEmbedAnnotations,
                    "The owner password grants full rights.");
        }
        { // User password, everything restricted
            const auto a = assessCapabilities(normal, userOpened({false, false, false, false, true, true}), same);
            require(a.capabilities.canRead && !a.capabilities.canEmbedOutline && !a.capabilities.canEmbedAnnotations &&
                        a.capabilities.mutation == MutationSafety::permissionRestricted &&
                        has(a, CapabilityReason::outlineNotPermitted) && a.canKeepLocalResearch,
                    "Restricted PDFs read, keep local research and never embed.");
        }
        { // Partial permission: annotations allowed, outline not
            const auto a = assessCapabilities(normal, userOpened({false, true, true, false, true, true}), same);
            require(!a.capabilities.canEmbedOutline && a.capabilities.canEmbedAnnotations &&
                        a.capabilities.mutation == MutationSafety::permissionRestricted,
                    "Annotation permission alone allows annotations only.");
        }
        { // Assemble permission alone allows bookmarks
            const auto a = assessCapabilities(normal, userOpened({false, false, false, true, true, true}), same);
            require(a.capabilities.canEmbedOutline && !a.capabilities.canEmbedAnnotations,
                    "Assemble permission allows outline changes.");
        }
        { // Copying text not permitted: no search
            const auto a = assessCapabilities(normal, userOpened({true, true, true, true, false, true}), same);
            require(!a.capabilities.canSearchText, "Extraction-restricted PDFs are not text-searched.");
        }
        { // Read-only file and read-only folder
            FileFacts file;
            file.readOnlyAttribute = true;
            auto a = assessCapabilities(file, inspected(), same);
            require(a.capabilities.canRead && !a.capabilities.canEmbedOutline &&
                        a.capabilities.mutation == MutationSafety::filesystemReadOnly &&
                        a.capabilities.availability == Availability::readOnly && has(a, CapabilityReason::fileReadOnly),
                    "A read-only file is local mode.");
            file = {};
            file.folderWritable = false;
            a = assessCapabilities(file, inspected(), same);
            require(!a.capabilities.canEmbedOutline && has(a, CapabilityReason::folderReadOnly),
                    "Safe save needs a writable folder.");
        }
        { // Sharing-locked
            FileFacts file;
            file.sharingLocked = true;
            const auto a = assessCapabilities(file, inspected(), same);
            require(a.capabilities.canRead && a.capabilities.mutation == MutationSafety::sharingLocked &&
                        !a.capabilities.canEmbedOutline,
                    "A locked file can be read; writes wait.");
        }
        { // Signed and certified default to local
            PdfFacts pdf = inspected();
            pdf.hasSignatures = true;
            auto a = assessCapabilities(normal, pdf, same);
            require(a.capabilities.mutation == MutationSafety::signedOrCertified && !a.capabilities.canEmbedOutline &&
                        has(a, CapabilityReason::signatures),
                    "Signed PDFs are never changed by default.");
            pdf.certified = true;
            a = assessCapabilities(normal, pdf, same);
            require(has(a, CapabilityReason::certified) && !has(a, CapabilityReason::signatures),
                    "Certification is reported as such.");
        }
        { // Malformed but readable
            PdfFacts pdf = inspected();
            pdf.malformed = true;
            const auto a = assessCapabilities(normal, pdf, same);
            require(a.capabilities.canRead && !a.capabilities.canEmbedOutline && has(a, CapabilityReason::malformed),
                    "Malformed PDFs read in recovery mode only.");
        }
        { // Unreadable and unsupported security
            PdfFacts pdf = inspected();
            pdf.readable = false;
            auto a = assessCapabilities(normal, pdf, same);
            require(!a.capabilities.canRead && a.capabilities.availability == Availability::unreadable,
                    "Unreadable PDFs are reported.");
            pdf = inspected();
            pdf.encryption = Encryption::unsupportedHandler;
            a = assessCapabilities(normal, pdf, same);
            require(a.capabilities.availability == Availability::unsupported && has(a, CapabilityReason::unsupportedSecurity),
                    "Unsupported security is never circumvented.");
        }
        { // Missing, offline, cloud placeholder: keep research, no reading
            FileFacts file;
            file.exists = false;
            auto a = assessCapabilities(file, inspected(), same);
            require(a.capabilities.availability == Availability::missing && a.canKeepLocalResearch,
                    "Missing files keep research.");
            file = {};
            file.offline = true;
            a = assessCapabilities(file, inspected(), same);
            require(a.capabilities.availability == Availability::offline && !a.capabilities.canRead, "Offline.");
            file = {};
            file.cloudPlaceholder = true;
            a = assessCapabilities(file, inspected(), same);
            require(a.capabilities.availability == Availability::cloudPlaceholder && !a.capabilities.canRead,
                    "Cloud placeholders are not downloaded silently.");
        }
        { // Changed externally while open: reconcile first
            RevisionFacts changed;
            changed.changedExternally = true;
            const auto a = assessCapabilities(normal, inspected(), changed);
            require(a.capabilities.mutation == MutationSafety::externalConflict && !a.capabilities.canEmbedOutline &&
                        a.reasons.front() == CapabilityReason::changedExternally,
                    "An external change blocks writes first.");
        }
        { // Several reasons are all reported, most important first
            FileFacts file;
            file.readOnlyAttribute = true;
            PdfFacts pdf = userOpened({false, false, true, false, true, true});
            pdf.hasSignatures = true;
            const auto a = assessCapabilities(file, pdf, same);
            require(a.reasons.front() == CapabilityReason::signatures && has(a, CapabilityReason::fileReadOnly) &&
                        has(a, CapabilityReason::outlineNotPermitted),
                    "Every reason is listed.");
            require(reasonName(CapabilityReason::passwordRequired) == "passwordRequired", "Reason names are stable.");
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "capability policy tests passed\n";
    return 0;
}
