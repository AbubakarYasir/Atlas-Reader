# Atlas Reader — Checkpoints to 1.0

This is the operational delivery plan for Atlas Reader. `PLAN.md` defines broad product and engineering requirements; `docs/CORE_WORKFLOWS.md` defines the detailed Library/Index → Reader → Bookmarks capability, fallback, conflict, portability, and recovery contract; this file defines implementation order, verification, owner handoff, and acceptance.

## Current checkpoint

| Field | Value |
|---|---|
| Checkpoint | **C1 — Reader workspace** |
| Target release | `0.8.0-beta.2` (build 4) |
| Status | **Ready for owner test** |
| Automated evidence | `dart format`: clean; `flutter analyze`: 0 issues; `flutter test`: 56 passed and 2 optional audits skipped; Windows integration: 3 passed; Windows release build: succeeded |
| Build | `build\windows\x64\runner\Release\atlas_poc.exe` |
| Hardware still required before 1.0 | Pen, touch, representative HDD, and physical printer |
| Previous owner decision | C0 accepted on 2026-09-09 |

The new core-workflow requirements do **not** reopen C1. C1 remains at its existing owner stop gate. After C1 is accepted, C2 implements the new core resilience contract before broader annotation/printing/metadata work proceeds.

## Operating contract

The only valid statuses are **Not started**, **In progress**, **Ready for owner test**, **Accepted**, and **Blocked**.

1. Work on one checkpoint only. Do not begin the next checkpoint until the owner explicitly marks the current checkpoint **Accepted**.
2. A handoff must include the executable or installer path, exact automated commands and results, required copied test fixtures, known limitations, and the numbered owner test in this file.
3. A failed owner test returns the checkpoint to **In progress**. Fix the same checkpoint, rerun its affected tests and the common quality gate, and hand it back for another owner test.
4. Every checkpoint must pass formatting, analysis, unit/widget tests, relevant PDF round-trip tests, the Windows integration suite, and a release build. Add focused regression coverage for its new behavior.
5. Use copies of personal PDFs for every write test. A skipped personal-file audit is not passing evidence.
6. Arabic strings, RTL layout, keyboard access, semantics, safe PDF replacement, conflict detection, and preservation of unrelated PDF data are continuous gates for new work.
7. Any work touching Library/Index, Reader, Bookmarks/Outline, document save/sync, permissions, passwords, signatures, document identity, or external-file conflict must satisfy `docs/CORE_WORKFLOWS.md`.
8. Atlas never bypasses PDF password/permission security, silently mutates a signed/certified original when that can invalidate its integrity, or overwrites a newer external revision.
9. A non-writable document is not equivalent to a failed research workflow. Where the checkpoint promises local fallback, Atlas must preserve work locally and explain whether data is Embedded, Local only, Pending embed, Local override, Locally hidden, or Conflict.
10. Desktop acceptance may advance C1–C8. Stable 1.0 remains blocked until C9 records pen, touch, HDD, and physical-printer evidence.
11. The Windows installer may be unsigned. Document the expected SmartScreen warning; signing is a later certificate- and funding-dependent enhancement.

### Common automated gate

Run these commands from `atlas_poc` and record their results in the checkpoint record before changing its status to **Ready for owner test**:

```powershell
dart format --output=none --set-exit-if-changed lib test integration_test
flutter analyze
flutter test
flutter test integration_test/windows_reader_workflow_test.dart -d windows
flutter build windows --release
```

Run applicable focused tests in addition to this common gate. The two opt-in real-document audits require copied Arabic and English PDFs:

```powershell
$env:ATLAS_AUDIT_ARABIC_PDF = 'C:\path\to\arabic-copy.pdf'
$env:ATLAS_AUDIT_ENGLISH_PDF = 'C:\path\to\english-copy.pdf'
flutter test test/runtime_pdf_audit_test.dart
```

From C2 onward, focused test fixtures must also cover the capability states introduced by that checkpoint. Passwords used in fixtures are test-only values and must never be committed as secrets or copied from personal documents.

## Checkpoint ledger

| ID | Release | Checkpoint | Status | Owner approval |
|---|---|---|---|---|
| C0 | `0.8.0-beta.1` | Baseline acceptance | **Accepted** | PASS — 2026-09-09 |
| C1 | `0.8.0-beta.2` | Reader workspace | **Ready for owner test** | Pending |
| C2 | `0.8.0-beta.3` | Core resilience and complete bookmarks | **Not started** | — |
| C3 | `0.8.0-beta.4` | Text markup and notes | **Not started** | — |
| C4 | `0.8.0-beta.5` | Advanced annotations | **Not started** | — |
| C5 | `0.8.0-beta.6` | Windows printing | **Not started** | — |
| C6 | `0.8.0-beta.7` | Covers and library details | **Not started** | — |
| C7 | `0.8.0-beta.8` | Portable metadata and app backup | **Not started** | — |
| C8 | `0.9.0-beta.1` | Arabic, RTL, and accessibility | **Not started** | — |
| C9 | `0.9.0-beta.2` | Performance and hardware qualification | **Not started** | — |
| C10 | `1.0.0-rc.1` → `1.0.0` | Installer and release candidate | **Not started** | — |

## C0 — Baseline acceptance

**Goal:** Establish the focused Library → Bookmark → PDF Ink workflow as the accepted starting point for all later checkpoints.

**Implementation exit:** Documentation reports the current verified results, and the release executable builds successfully. No feature work is included.

**Required fixtures:** A writable copy of a PDF with a nested outline, plus a folder containing at least one Arabic-titled and one English-titled PDF.

**Owner test:**

1. Run `build\windows\x64\runner\Release\atlas_poc.exe`.
2. Use **Open PDF** on the copied PDF outside every registered library folder; confirm it appears in **Recents** without enrolling its parent folder.
3. Add the fixture folder, scan it, and confirm both Arabic and English books appear and can be filtered.
4. Use `Ctrl+K` to find a nested bookmark and confirm Atlas opens its exact page.
5. Enter **Write**, draw a mouse stroke, save, close, and reopen the document; confirm the stroke remains on its source page.
6. Confirm the original outline and metadata still exist and the sibling `.atlas-backup` recovery file was created by the save.

**Stop gate:** Record PASS and the approval date in the ledger, or record each failure under the template below. Do not start C1 before explicit PASS.

## C1 — Reader workspace

**Goal:** Make the existing reader and workspace prototypes one dependable desktop reading surface.

**Implementation exit:** The reader has a persistent, collapsible **Outline/Pages/Bookmarks/Annotations** panel with search and selected-page state; lazy real thumbnails; 10–6400% free zoom plus fit-page and fit-width; Write-mode pan/zoom; equal library cards with generated PDF covers; accessible book/tab context menus; and optional tab restoration (off by default).

**Owner test:** Follow the numbered corrective C1 checklist in this record.

**Stop gate:** All navigation paths land on the requested page, session state restores, no control obscures content, and the workflow passes at 200% scale.

### Acceptance record — C1

- Status: Ready for owner test
- Commit: C1 reader-workspace candidate on `main`
- Version/build: `0.8.0-beta.2+4`
- Executable: `build\windows\x64\runner\Release\atlas_poc.exe`
- Automated commands and results: formatting clean; analysis 0 issues; 56 unit/widget tests passed; 2 personal-file audits skipped; 3 Windows integration scenarios passed; release build succeeded
- Focused tests: 10% under-fit zoom, fit/custom transitions, Outline-first navigation, real page previews and PDF covers, fixed card geometry, lazy tab sessions, context actions, Arabic UI, and opt-in tab restoration
- Fixture copies required: three PDFs, including one mixed Arabic/English PDF with an outline and one writable PDF copy for Write-mode testing
- Known limitations: PDF is the only readable/writable format; the two personal-file audits remain unrun without owner-supplied fixture copies
- Hardware evidence supplied: desktop mouse/keyboard only; pen, touch, HDD, and physical printer remain required at C9
- Owner result: Pending corrective build retest
- Defects found: Build 3 lag, constrained zoom, missing covers, uneven cards, Pages-first navigation, and missing context actions were corrected in build 4
- Approval date: Pending

### Corrective owner checklist

1. Open the previously lagging book and confirm the interface stays responsive.
2. Open three PDFs and switch repeatedly without pauses or lost state.
3. Confirm Outline is first and selected by default; Pages shows real previews.
4. Zoom below Fit down to 10%, enter custom values, and switch Read/Write modes.
5. Right-click books in every library view and use each available action.
6. Right-click active/inactive tabs; test Close, Close others, and Close right.
7. Create unsaved ink and confirm closing cannot silently discard it.
8. Test Shift+F10/Menu-key menus and Arabic layout.
9. Rescan and confirm covers appear without freezing and cards remain equal.
10. In Settings, verify tab restoration is off by default, then enable it and confirm tabs return after restarting Atlas.

## C2 — Core resilience and complete bookmarks

**Goal:** Make Library/Index → Reader → Bookmarks dependable on real-world documents, not only ordinary writable PDFs. Complete bookmark management, introduce explicit document capability states and local fallback, and make bookmark research exportable/recoverable.

C2 follows the detailed contract in `docs/CORE_WORKFLOWS.md` and is implemented in four internal subgates. These are implementation order, not separate releases; C2 remains **In progress** until all four pass together.

### C2.0 — Index and document-identity hardening

**Implementation exit:**

- Library records distinguish Available, Locked/encrypted, Read-only/restricted, Offline source, Missing, Cloud placeholder/not local, Unreadable/corrupt, Changed since last index, and Unsupported states where detectable.
- Multiple and nested roots do not create duplicate logical records.
- Reparse points/junctions cannot create recursive scan loops.
- Permission-denied subfolders and individual bad files do not abort the scan.
- A root that cannot be enumerated (disconnected drive/network path, unavailable source) does not trigger destructive removal of its indexed books.
- Safe rename/move matching preserves favorites, progress, and local bookmark state; ambiguous copies remain separate instead of stealing each other's data.
- Files changing/disappearing during scan are retried/deferred without deleting unrelated records.

**Focused fixtures:** nested roots; junction cycle; permission-denied folder; encrypted PDF; corrupt-but-detectable PDF; removable root disconnected after indexing; renamed book; identical copied book.

### C2.1 — Capability and local-overlay foundation

**Implementation exit:**

- Opening and mutation are separate capabilities.
- Open-password and permissions-password flows are distinguished.
- Passwords are never stored in SQLite/logs/exports; session memory only is allowed pre-1.0.
- Permission-restricted, filesystem-read-only, transiently locked, and signed/certified PDFs can still use the promised Atlas-local bookmark workflow.
- Storage state is explicit: Embedded, Local only, Pending embed, Local override, Locally hidden, Conflict.
- Embedded bookmark edits made while the source cannot be changed become local overrides; deleting an embedded bookmark locally becomes an explicit local suppression, not a false claim that the PDF changed.
- Save/commit rechecks source revision/capability. A PDF changed outside Atlas blocks blind overwrite and opens reconciliation.
- The user can choose Retry, Keep local, Save writable/working copy, Locate file, or Export changes as appropriate.
- A source removed after the session opened does not destroy current local bookmark work.

### C2.2 — Complete bookmark editor and reconciliation

**Implementation exit:**

- Users can create folders/bookmarks; rename, delete, move, reparent, reorder, and nest them; edit destinations; and search them.
- Sibling order, deep hierarchy, duplicate visible names under different parents, page destinations, Unicode/Arabic/Urdu titles, and mixed direction text are preserved.
- Find/reveal current bookmark is available.
- Tree editing is keyboard and screen-reader complete and works at 200% scale in English and Arabic.
- When a formerly restricted/read-only/locked document becomes writable, Atlas re-reads it and previews promotion of local changes into the PDF.
- External outline edits and local edits reconcile through a diff. Non-overlapping unambiguous changes may merge; ambiguous same-node changes never choose a winner silently.
- A second sync after successful commit reports zero diff.

### C2.3 — Bookmark portability and recovery

**Implementation exit:**

- Export all bookmarks, selected subtree, or local-only changes.
- Lossless versioned `.atlas-bookmarks.json` export preserves hierarchy, destination, descriptions/tags when present, storage origin, and guarded document identity metadata without passwords.
- Markdown and CSV exports provide readable/research-friendly output.
- Atlas bookmark bundles import with preview and support append-at-root, append-under-selection, merge, and explicitly confirmed replace.
- Import can remain local-only even on a writable PDF.
- The UI never labels an export format as Foxit-compatible until round-trip fixture tests prove that compatibility; Foxit XML support is an investigated follow-up rather than an assumed standard.
- Local-only data has an explicit export/recovery path and cannot be trapped solely in an undocumented database workflow.

### C2 owner test

Use copied fixtures only. At minimum test:

1. **Normal writable mixed Arabic/English PDF:** create a three-level tree with duplicate names under different parents; rename, reorder, move, nest, delete, save twice, reopen in Atlas and another standard PDF reader; both readers agree and second sync is zero-diff.
2. **Permissions-restricted PDF without permissions password:** read it, create/edit/hide bookmarks locally, restart Atlas, verify work survives and is clearly Local only; external PDF remains unchanged.
3. **Permissions-restricted PDF with valid test authorization:** create local work first, then authorize and promote it; preview is correct and the committed outline round-trips externally.
4. **Filesystem read-only PDF:** local work succeeds; **Save writable copy** creates a separate editable copy without altering the original.
5. **Transient Windows sharing lock:** make local changes while another process holds the source; first embed attempt fails safely; release the lock, Retry, and confirm no work was lost.
6. **Signed/certified fixture:** Atlas warns that modifying may affect document integrity, defaults to local/working-copy behavior, and does not silently rewrite the signed original.
7. **External-change conflict:** modify the outline in another app after Atlas opens the document; Atlas blocks overwrite and shows reconciliation before commit.
8. **Offline/removable source:** index a book and create local research data, disconnect the source, rescan, and confirm the book/research state is retained as Offline rather than purged; reconnect and relink safely.
9. **Export/import:** export selected subtree and full tree to Atlas JSON, Markdown, and CSV; import Atlas JSON into a clean local state; hierarchy, order, Unicode, destinations, and supported metadata match.
10. **Index edge cases:** scan overlapping roots, a junction-cycle fixture, one unreadable file, one encrypted file, a renamed file, and an identical copy; the UI remains responsive and research state attaches only to the correct logical document.

**Stop gate:** Every required capability state has a tested, localized, accessible outcome; no protected/signed/offline/conflicted case loses research data or silently mutates the wrong source; bookmark round-trip is interoperable on writable PDFs; local-only work is exportable and recoverable.

## C3 — Text markup and notes

**Goal:** Promote text markup and sticky-note prototypes into the focused reader as reliable standard PDF annotations.

**Implementation exit:** Highlight, underline, strikeout, and sticky-note tools support selection, editing, deletion, undo/redo, and page isolation, with portable save/reopen behavior. These tools reuse the C2 document-capability and external-revision preflight. If a source cannot safely accept a standard PDF annotation, Atlas must not pretend the annotation was embedded; any local/pending fallback must be explicitly labeled and recoverable.

**Owner test:** Add every type on multiple pages, edit and delete samples, use undo/redo, save, and reopen in Atlas and a standard reader. Confirm annotations stay on the correct pages and untouched pages, outlines, metadata, and existing annotations remain unchanged. Repeat the save attempt on a restricted/read-only fixture and verify the C2 safety contract is honored.

**Stop gate:** Every portable type round-trips in both readers without collateral loss, and non-writable cases preserve user work or decline the mutation clearly without false success.

## C4 — Advanced annotations

**Goal:** Complete editable drawing objects and annotation discovery.

**Implementation exit:** Free text, callouts, lines, arrows, rectangles, ellipses/clouds, move, resize, style, and delete work through standard PDF representations. The annotation panel lists, searches, filters, sorts, and jumps to exact locations with keyboard and screen-reader access. All saves reuse C2 capability/conflict preflight.

**Owner test:** Create and modify every type on multiple pages; search and filter the panel; jump to each result; perform the workflow without a mouse; save and reopen externally; confirm unrelated annotations survive. Repeat one operation against a conflicted/restricted source to verify no blind overwrite.

**Stop gate:** All objects remain editable after reopen, panel jumps are exact, the complete workflow is keyboard accessible, and C2 safety behavior remains intact.

## C5 — Windows printing

**Goal:** Print locally through Windows with predictable layout and no document mutation.

**Implementation exit:** Native printer selection and preview support all, current, and custom ranges; page labels; odd/even and reverse order; copies and collation; orientation and paper size; fit, actual size, custom scale, shrink, multi-up, booklet, grayscale, and margins. The preview shows a clear summary.

**Owner test:** Hash a copied source PDF, exercise every setting through Microsoft Print to PDF, inspect output page selection/order/layout, cancel one job, and confirm the source hash never changes. Track a physical-printer run as pending C9 hardware evidence.

**Stop gate:** Desktop test passes and no print or cancel path changes the PDF.

## C6 — Covers and library details

**Goal:** Give every discovered book a useful cover and accurate read-only details without weakening C2 index identity/availability behavior.

**Implementation exit:** Atlas prefers a supported embedded cover/thumbnail, renders PDF page one as the deterministic fallback, invalidates stale cache entries, and displays complete bibliographic, technical, capability, and reading facts with portable and local-only values clearly distinguished. Locked/offline/unreadable states have deterministic fallbacks rather than broken cards.

**Owner test:** Scan PDFs with and without embedded thumbnails plus an EPUB; include encrypted/read-only/offline/unreadable states; inspect image/state accuracy and refresh behavior; then rename, move, modify, copy, and rescan files and confirm covers/details/research data follow only the correct books.

**Stop gate:** Every readable PDF has a correct cover or deterministic fallback, every unavailable/restricted book has a useful state, and no stale cover/metadata/research state is associated with the wrong document.

## C7 — Portable metadata and app backup

**Goal:** Edit portable bibliographic/research metadata safely and make app-local research state explicitly backupable before 1.0.

**Implementation exit:** The metadata sheet supports field-level reset, validation, before/after preview, and explicit **Save into PDF**. It maps core fields to document information/XMP, preserves unknown namespaces, and labels local-only fields. It uses the standard capability preflight and validated temporary-save/backup path.

Atlas also provides a versioned **Export Atlas Backup** / restore workflow for app-local research state and configuration needed for recovery. Source PDFs are excluded by default. Restore does not relink data by filename alone; unresolved books are previewed for safe relinking.

**Owner test:** On copied Arabic and English PDFs, edit portable fields, cancel one attempt, save another, and reopen in Atlas and an external metadata inspector. Verify outlines, annotations, unknown XMP, page content/count, and backup recovery remain intact. Then export app backup, restore into a clean test profile, and verify local-only bookmark overlays, descriptions/tags, favorites, progress, and folder configuration return without being attached to an ambiguous wrong copy.

**Stop gate:** Both metadata inspectors agree on portable fields, preservation checks pass, and local-only research data has a documented tested backup/restore path.

## C8 — Arabic, RTL, and accessibility

**Goal:** Complete the Windows accessibility and Arabic release contract across all work delivered through C7.

**Implementation exit:** All user-facing strings are localized; CI rejects new hard-coded UI strings; locale-aware direction, bidi isolation, Arabic search normalization and documented sorting, localized semantics/announcements, visible focus, high contrast, Narrator, and 100%/200% scaling are covered by automated and manual evidence. Earlier checkpoints must add English and Arabic resources for each new UI rather than waiting for C8.

This includes capability banners, password prompts, storage-state badges, local-only notices, conflict/reconciliation previews, import/export, backup/restore, and offline/missing states introduced in C2/C7.

**Owner test:** In Arabic, complete library, reader, bookmark, restricted-document fallback, conflict, import/export, annotation, metadata, backup, and print workflows using keyboard and Narrator at 100% and 200% scale. Verify mixed Arabic/English/Urdu titles, filenames, paths, identifiers, dates, page labels, numbers, and capability states remain readable and correctly ordered.

**Stop gate:** The published Arabic/RTL/accessibility matrix has no failed or untested core workflow.

## C9 — Performance and hardware qualification

**Goal:** Prove Atlas meets responsiveness, identity, recovery, and Windows hardware targets before release-candidate packaging.

**Implementation exit:** A reproducible benchmark fixture and results cover a large library, overlapping roots, unavailable roots, a long document, 10,000 bookmarks, restricted/read-only/conflicted saves, interrupted writes, mouse, touch, pen, SSD, HDD, and physical printing. Test output has no unexplained warnings, and regressions have enforceable thresholds where practical.

**Owner test:** Run the published matrix on representative hardware; confirm 10,000-bookmark search is below 100 ms p95, progressive scans keep the UI responsive, offline roots are not purged, interrupted/conflicted saves recover without corruption, pen/touch strokes are page-isolated, and a physical print matches its preview.

**Stop gate:** Block C10 and stable 1.0 while any required hardware evidence is missing or any threshold/core resilience case fails.

## C10 — Installer and release candidate

**Goal:** Ship an owner-approved, reproducible Windows 1.0 artifact.

**Implementation exit:** An unsigned Windows installer supports clean install, upgrade, uninstall, and optional user-controlled `.pdf` association. It includes licenses, release notes, capability/local-only guidance, bookmark export/import guidance, backup/recovery guidance, SmartScreen expectations, and CI-produced artifacts. Installation and uninstall never remove user books or silently discard Atlas-local research data.

**Owner test:** Test a clean install and an upgrade from the accepted baseline; launch normally and through **Open with**; decline and then enable association; confirm the database, PDFs, and local-only research data persist; export a backup; uninstall and verify user books remain; reinstall, restore/reopen, and run the complete regression checklist.

**Stop gate:** Publish `1.0.0-rc.1`, obtain explicit owner approval, and promote the identical accepted artifact to `1.0.0`. Any code or packaging change after approval requires a new release candidate and regression pass.

## Acceptance record template

Copy this section beneath the active checkpoint when handing it to the owner:

```markdown
### Acceptance record — C#

- Status: Ready for owner test
- Commit:
- Version/build:
- Executable or installer:
- Automated commands and results:
- Focused tests:
- Fixture copies required:
- Known limitations:
- Hardware evidence supplied:
- Owner result: Pending
- Defects found:
- Approval date:
```

## Fixed boundaries

Windows 11 is the primary 1.0 platform. The roadmap promises no dates. TTS, speed-reading in the primary UI, cloud accounts/synchronization, EPUB rendering or ink, OCR, arbitrary PDF text/object editing, forms, signature creation, conversion, hosted sharing, multimedia, and password/restriction bypass remain explicitly deferred.

Competitor behavior is a research input, not a scope mandate. Adobe Acrobat and Foxit PDF Editor are references for mature PDF security/bookmark workflows; Librera is a reader-first reference; Xournal++ is a pen-first/local-layer reference. Atlas remains focused on its own core: **Index → Reader → Bookmarks**, with portability, local fallback, Arabic/RTL, accessibility, and recovery built into those workflows.