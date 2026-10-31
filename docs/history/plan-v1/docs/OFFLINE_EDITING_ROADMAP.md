# Offline editing and signatures — required before Windows 2.0

Owner-confirmed scope, 2026-09-27. Planned only; no implementation or compatibility claim.

## Scope and order

The owner approved offline original PDF text/image editing, page organization, form tools, secure redaction and equations, with offline signatures in later stages. OCR and audio notebooks are excluded. Online accounts, sync, shared reviews, collaboration, cloud AI, uploads, remote document resources and update checks are excluded from the 2.0 runtime. This is a substantial scope expansion, not a promise to retain an earlier release date.

Order: N3 → N4 → N5 → N6 → N7 → F1 → F2 → F3 → F4 decision → E1 → E2 → E3 → E4 → E5 → N8 → N9 → N10 → N11. N3 remains the only active checkpoint. Each E stage requires automated evidence, rendered/keyboard/Arabic review, a checksummed package and explicit `E# PASS`. No feature can be silently removed because implementation is difficult.

N6 owns complete annotation control; N7 owns ordinary existing AcroForm filling. The E stages extend these capabilities rather than replacing their tests. Added editing applies to PDF, not EPUB/Kindle source editing. This scoped list does not imply every offline feature of every competing application, Office conversion, executable plugins, XFA/JavaScript or a full notebook authoring system.

## Common qualification gate

### Actionable implementation checklist for every stage

1. Open the stage only after its predecessor's owner PASS; create the one active named checkpoint branch from the accepted predecessor. Record stage owner as the implementing maintainer and acceptance owner as the repository owner.
2. Create a stage plan/evidence sheet with numbered requirements, dependency decisions, fixtures with provenance/hash, tests, budgets and explicit exclusions. Link each requirement to its implementation task and test; all start Pending.
3. Inventory the accepted APIs against required operations. Compare at least two credible open-source routes when current dependencies lack the capability, or document why only one is feasible. No dependency is selected merely by this roadmap.
4. Run bounded, disposable-fixture probes for the highest-risk operations before UI work. Record correct output and failure/preservation behavior, not just successful builds. Reject an unsafe route before integrating it.
5. Approve the capability/dependency ADR, including exact version/source, build reproducibility, transitive licenses/fonts/codecs, notices/source/relinking obligations, platform portability and replacement path. Keep MIT Atlas-owned code and accepted Qt/qpdf responsibilities intact. License uncertainty blocks adoption; do not treat open-source availability as automatic compatibility.
6. Implement domain/adapter operations and failure-injection tests first; add UI using existing primitives, clear previews and capability states. Update the operation matrix whenever a limitation is found.
7. Run automated and independent-reader tests, then rendered wide/narrow/200%/RTL/keyboard/Narrator review. Record exact commit, fixture, command, expected/actual result and evidence path. All required manual skips block acceptance.
8. Commit code/tests/docs together, push, verify exact-head CI, provide one checksummed owner package with a short numbered test sheet, and wait for explicit stage PASS. Only then merge/close the stage. Never mark a task done from documentation alone.

### Dependency decision queue (not selected libraries)

| Stage | Required decision/probe | Reject route if |
|---|---|---|
| E1 | Qualify page-tree transformations using accepted qpdf route first; define reference remapping in Atlas | Outlines/forms/local notes cannot be preserved or explicitly reconciled |
| E2 | Qualify editable PDF content/font/image operations; separately select a sandboxed offline equation renderer | Arabic shaping/encoding breaks, required fonts cannot be distributed, or equation input can execute arbitrary code |
| E3 | Reuse N7 qualified form/appearance writer; qualify field authoring and accessibility | Saved values and visible appearances disagree or field semantics are lost |
| E4 | Select actual content-removal/sanitization route, including images and prior revisions | Targeted content survives extraction/object/image recovery, even if rendering looks correct |
| E5 | Qualify PDF signing/validation library and Windows key-store adapter behind portable contracts | Key material leaks, unsupported trust is called valid, or the library requires an online service |

No paid/proprietary SDK, hosted renderer, cloud signing service or license change is authorized by this plan. A failed open-source feasibility gate requires evidence and an owner decision, not an unannounced substitution.

Before each E stage implements a write operation, qualify an open-source route on licensed disposable fixtures and record an ADR: exact dependency/source/license, operation limits, preservation, threat model, resource budgets and rollback. N2's accepted Qt PDF/qpdf split is not evidence that those engines already provide content editing, forms, redaction or signing. Add capabilities behind Atlas-owned adapters; do not rewrite accepted engines without evidence. An unavailable safe route blocks the requirement until a solution or explicit owner scope revision is recorded.

All writes reuse N5 identity/revision/permissions checks, temporary output validation, recovery and reopen. Default new-copy output for structural/content changes. No destructive work on owner originals in testing. Use distinct commands for annotations, original-content editing, page changes and redaction. Unsupported objects remain intact or the operation is refused with a specific explanation; no silent rasterization, font substitution or loss of editability. PDF coordinate tests cover rotated/cropped/mixed-size pages and zoom/DPI. Source and output identities, bookmarks, notes, progress and references must be reconciled explicitly after page changes.

## E1 — Page organization (`2.0.0-beta.9`)

### Work

Reorder, insert blank pages, duplicate, delete, extract, split, merge, persist rotation and crop with before/after preview, undo before commit and save to a new file by default. Choose page ranges visually and by labels without confusing labels with physical indexes. Define treatment of outlines, links, annotations, forms and Atlas-local destinations before committing; show unresolved references, never silently move research to a different page. Distinguish reversible display crop from stored crop and from redaction.

### Automated QA

Mixed boxes/rotations, page labels, deep outlines, cross-page links, duplicate form names, annotations and local overlay relocation; empty/invalid ranges; merge conflicts; interruption/locks/external revision; source-hash invariance for new-copy output. Validate page trees and independently reopen outputs. Cropping must not be described as content removal.

### Owner test

On copies, merge two books, reorder/delete/extract a range, undo, then save/reopen. Inspect affected bookmarks, links, comments and page labels in Atlas and an independent reader, including Arabic and rotated pages.

**Stop gate:** wrong page/reference reassociation, unexplained lost objects, unsafe source overwrite or missing `E1 PASS` blocks E2.

## E2 — Original content and equations (`2.0.0-beta.10`)

### Work

Edit existing text and images: select text runs, replace/insert/delete text, adjust supported font/size/color/alignment; insert/replace/move/resize/crop/delete images. Support Arabic/Urdu shaping, bidi and font fallback choices with explicit preview; scanned text stays an image because OCR is excluded. Handle embedded subset/missing fonts honestly. Unsupported text encodings/layouts must never produce silent corruption.

Add an offline equation editor with source plus rendered preview, edit/re-render, move/resize, and portable PDF output. Store versioned editable equation source locally and include it in N8 backup/export; explain that ordinary PDF readers may see only the rendered equation. A bundled or optional local renderer must have explicit installation/license requirements and remain offline. Disable shell escape, external commands, arbitrary file access and resource/network fetch in equation input; reject malicious and resource-exhausting expressions.

### Automated QA

Text extraction and visual before/after comparisons, Arabic/Urdu joining/diacritics, subset/missing fonts, line wrapping, image masks/transparency, untouched-content preservation and cross-reader reopen. Equation input round-trip, rendering limits, injection/path tests and absent-renderer error state. Undo/recovery, source safety, keyboard/scaling and memory/time budgets.

### Owner test

Edit English and Arabic/Urdu text, replace an image, insert and re-edit an equation, cancel/undo and reopen the new output in a second reader. Confirm original files remain unchanged and unsupported scanned-text editing is explained.

**Stop gate:** content damage, silent font/encoding loss, unsafe equation execution or missing `E2 PASS` blocks E3.

## E3 — Offline form tools (`2.0.0-beta.11`)

### Work

Extend N7 filling with AcroForm creation/editing: text/check/radio/choice fields, labels/tooltips, names, tab order, required/read-only flags and defaults; move/resize/delete fields with preview. Define supported non-scripted validation without implying JavaScript calculation support. Create signature fields for E5 without signing early. Preserve editable values and appearance streams; do not silently flatten. XFA, document scripts, network submission and hosted signing remain excluded with explicit notices.

### Automated QA

Field names/groups/values/defaults, duplicate names, keyboard tab order and accessible labels; Arabic/Urdu values/appearances; required-field errors; save/cancel/reopen and independent reader editing; permissions/signed-document policy; form structure preservation during E1/E2 operations. No submit/script/network side effects.

### Owner test

Create and fill a small mixed-language form with each supported field type, navigate by keyboard/Narrator, correct validation errors, save a copy and edit its values in an independent reader.

**Stop gate:** lost values, mismatched visible/actual values, broken field access or missing `E3 PASS` blocks E4.

## E4 — Secure redaction (`2.0.0-beta.12`)

### Work

Separate mark-for-redaction from irreversible Apply to new copy. Support selected text and rectangular regions, including image pixels. Preview scope and explain that highlights, crops and black rectangles do not remove data. Remove recoverable underlying content from the output, including relevant hidden text, annotations, metadata and embedded content; make sanitization choices explicit and conservatively refuse unsupported structures. Do not promise anonymization beyond the tested sanitization scope. Keep originals and backups clearly separate from the sanitized output; warn they retain removed content.

### Automated QA

Synthetic secrets in text, images, hidden layers, metadata, comments, attachments, compressed objects and incremental revisions. Test extraction/search/copy, decoded object inspection, image recovery and independent rendering against the sanitized output. Full rewrite must not leave prior revisions recoverable. Verify non-redacted content, Arabic/rotated areas, cancellation, destination collisions, interrupted writes and source preservation. No sensitive canaries in routine logs.

### Owner test

Mark and apply redactions to synthetic fixtures, inspect preview and sanitized output, verify removed text cannot be searched/copied/recovered with the independent checks, and verify retained content. Review warnings about retained originals/backups.

**Stop gate:** any recoverable targeted secret, ambiguous output identity, misleading secure-removal claim or missing `E4 PASS` blocks E5. Visual inspection alone cannot pass redaction.

## E5 — Offline signatures (`2.0.0-beta.13`)

### Work

Provide two distinct workflows: a visible handwritten/image signature mark and a cryptographic PDF signature using a user-supplied local certificate/private key. A visible mark is not cryptographic proof. Protect key/password access through platform adapters; never log, export in diagnostic bundles or silently persist private keys/passwords. Signing requires explicit intent and exact-document preview; no automatic signing.

Offline verification separates byte integrity, certificate chain/trust, time information and revocation evidence. Report unknown/unavailable states honestly when local evidence is insufficient. No online certificate enrollment, OCSP/CRL retrieval, trusted timestamp requests or signing service. Locally supplied evidence may be used only through a qualified validation route. Never call an offline signature universally trusted or legally sufficient. Support multiple-signature/incremental-update preservation only with fixture proof; earlier E operations must respect signed/certified state.

### Automated QA

Synthetic test certificates only: correct/incorrect passwords, tampering, expired/untrusted certificates, unavailable revocation, signed/certified restrictions, multiple signatures, interruption and external changes. Independent reader verifies supported signatures; no existing signature silently invalidated. Network-observed execution, private-key/log/backup exclusion and local evidence provenance tests.

### Owner test

Add a visible mark, then separately sign a disposable document with a test certificate. Verify in Atlas and another reader, modify a copy to demonstrate tamper detection, and inspect clear offline trust/revocation limitations while disconnected.

**Stop gate:** false valid/trusted status, key exposure, unapproved network access, silent invalidation or missing `E5 PASS` blocks N8.

## Final release coverage

N8 includes editable equation sources, local annotation state, form drafts where retained, collections/tags and changed-page identity mappings in versioned recovery tests; private keys/passwords stay excluded. N9 repeats every E workflow for Arabic/Urdu, keyboard/Narrator and scaling. N10 repeats destructive-operation failure injection, hostile-input containment, offline operation and licensing review on the distributable. N11 cannot ship until E1–E5 are Accepted; an unsupported edge case must be documented and cannot excuse a missing core capability.
