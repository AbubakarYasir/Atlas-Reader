# N5 Bookmarks and Document Safety: Evidence Ledger

<!-- atlas-status: N5|in-progress -->

Evidence for N5 ([CHECKPOINTS.md](../../CHECKPOINTS.md#n5--bookmarks--document-safety-200-beta3)); contract in [DATA_AND_SAFETY.md](../DATA_AND_SAFETY.md). Entries are added as work lands; statuses use Planned · Implemented · Verified · Accepted · Released.

## Start — 2026-11-02

- Owner wrote `N4 PASS` on 2026-11-02; see [N4 acceptance](N4_ACCEPTANCE.md). `2.0.0-beta.2` is released from `main`.
- Stage branch `native-v2-n5-bookmarks-safety` opened from `main`. First sub-stage: N5.1 capability check.

## N5.1 Capability check — 2026-11-02

- `src/core/document/CapabilityPolicy` (pure C++): `assessCapabilities(FileFacts, PdfFacts, RevisionFacts)` → `DocumentCapabilities` + ordered reasons. Blocking order: external change, not inspected, malformed, certified/signed, read-only file, read-only folder, sharing lock; PDF permissions may allow outline (modify or assemble) and annotations separately; the owner password lifts permission flags; extraction-restricted PDFs are not text-searched. Tests: `atlas_capability_policy` (every row of the table, partial permissions, multiple reasons).
- `src/app/document/DocumentCapabilityCheck`: `QpdfInspector` runs `qpdf --json=2` (encrypt, acroform; trailer → catalog for `/Perms /DocMDP`), 20 s timeout, password via `--password-file=-` (never on the command line or disk); `FileFactsProbe` uses file attributes, a deny-write share probe (`CreateFileW`, read access only) and the folder ACL (no temporary files); `RevisionStamp` (size + UTC modification time). Tests: `atlas_qpdf_inspector` (parsing of recorded JSON; with qpdf: A001, A009 wrong/user/owner, A010, Unicode-named copy, external change, read-only copy where the account honours it, SHA-256 of sources unchanged). Local run with qpdf 11.9 passes; Windows CI uses the pinned 12.4.1 package.
- `ReaderController`: `capability` role, background check when a tab becomes ready, `recheckCapabilities(index)`; the first stamp is the baseline, a reopen accepts the file as it is.
- UI: status-bar chip and explanation popup (EN/AR/UR, 26 new strings, `check-strings` PASS 298 keys). Rendered: A010 shows *Signed* with "certified by its author".
- Packaging: CI downloads the SHA-256-pinned official qpdf 12.4.1 MSVC64 package, sets `ATLAS_QPDF` for tests and copies the same minimal runtime as the N2 selected route into `qpdf/` with its licence and notice.
- CI repair: the N2 content-fidelity workflow pins Python 3.12 (Pillow 11.3.0 had no wheel for the runner's newer Python).
