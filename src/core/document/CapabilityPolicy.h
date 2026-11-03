#pragma once

// N5.1 capability check: what Atlas may do with a document, from facts that
// adapters gather (file system, PDF security via qpdf, revision tracking).
// Pure C++; no engine, Qt or OS types. Opening and changing are separate
// capabilities (docs/DATA_AND_SAFETY.md, "Document capabilities"); local
// research is always allowed unless the document cannot be identified.

#include "core/document/DocumentCapabilities.h"

#include <cstdint>
#include <string>
#include <vector>

namespace atlas::document {

struct FileFacts final {
    bool exists{true};
    bool offline{false};           // on an unavailable volume or share
    bool cloudPlaceholder{false};  // contents not on this device (no silent download)
    bool readOnlyAttribute{false}; // the file itself is read-only
    bool folderWritable{true};     // safe save writes a temporary file beside it
    bool sharingLocked{false};     // another program holds it open for writing
};

enum class Encryption : std::uint8_t {
    none,
    openPasswordRequired,  // no password given yet, or a wrong one
    openedWithUserPassword,
    openedWithOwnerPassword, // full rights by the PDF's own rules
    unsupportedHandler,      // a security handler Atlas cannot open
};

// PDF permission bits as reported by the structural engine (meaningful only
// when encrypted and opened with the user password).
struct PdfPermissions final {
    bool modifyContents{true};
    bool modifyAnnotations{true};
    bool fillForms{true};
    bool assemble{true}; // insert/rotate/delete pages, create bookmarks
    bool extract{true};
    bool print{true};
};

// A signature form field (N5.3 Signatures panel). Atlas reports what the PDF
// declares; it does not check whether a signature is valid (after 2.0, Track F).
struct SignatureField final {
    std::string name;   // the field's full name, as the PDF gives it
    bool signedField{}; // the field holds a signature value
    int page{};         // 1-based page of its widget; 0 when unknown
};

struct PdfFacts final {
    bool inspected{false}; // false until the structural check has run
    bool readable{true};
    bool malformed{false}; // readable only after repair
    Encryption encryption{Encryption::none};
    PdfPermissions permissions;
    bool hasSignatures{false}; // signed signature fields present
    bool certified{false};     // DocMDP certification signature
    std::vector<SignatureField> signatureFields; // in the order the PDF lists them
};

struct RevisionFacts final {
    bool changedExternally{false}; // size/time/fingerprint differ from what Atlas last saw
};

enum class CapabilityReason : std::uint8_t {
    missing,
    offline,
    cloudPlaceholder,
    passwordRequired,
    unsupportedSecurity,
    unreadable,
    malformed,
    notInspected,
    outlineNotPermitted,
    annotationsNotPermitted,
    fileReadOnly,
    folderReadOnly,
    sharingLocked,
    signatures,
    certified,
    changedExternally,
};

struct CapabilityAssessment final {
    DocumentCapabilities capabilities;
    bool canKeepLocalResearch{true};
    std::vector<CapabilityReason> reasons; // most important first
};

[[nodiscard]] CapabilityAssessment assessCapabilities(const FileFacts& file, const PdfFacts& pdf,
                                                      const RevisionFacts& revision);

// Stable machine names ("passwordRequired", …) for logs, QML and tests.
[[nodiscard]] std::string reasonName(CapabilityReason reason);

} // namespace atlas::document
