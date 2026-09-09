# Atlas Reader — Checkpoints to 1.0

This is the operational delivery plan for Atlas Reader. `PLAN.md` defines the
product and engineering requirements; this file defines the order in which work
is implemented, verified, handed to the owner, and accepted.

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

## Operating contract

The only valid statuses are **Not started**, **In progress**, **Ready for owner
test**, **Accepted**, and **Blocked**.

1. Work on one checkpoint only. Do not begin the next checkpoint until the
   owner explicitly marks the current checkpoint **Accepted**.
2. A handoff must include the executable or installer path, exact automated
   commands and results, required copied test fixtures, known limitations, and
   the numbered owner test in this file.
3. A failed owner test returns the checkpoint to **In progress**. Fix the same
   checkpoint, rerun its affected tests and the common quality gate, and hand it
   back for another owner test.
4. Every checkpoint must pass formatting, analysis, unit/widget tests, relevant
   PDF round-trip tests, the Windows integration suite, and a release build.
   Add focused regression coverage for its new behavior.
5. Use copies of personal PDFs for every write test. A skipped personal-file
   audit is not passing evidence.
6. Arabic strings, RTL layout, keyboard access, semantics, safe PDF replacement,
   and preservation of unrelated PDF data are continuous gates for new work.
7. Desktop acceptance may advance C1–C8. Stable 1.0 remains blocked until C9
   records pen, touch, HDD, and physical-printer evidence.
8. The Windows installer may be unsigned. Document the expected SmartScreen
   warning; signing is a later certificate- and funding-dependent enhancement.

### Common automated gate

Run these commands from `atlas_poc` and record their results in the checkpoint
record before changing its status to **Ready for owner test**:

```powershell
dart format --output=none --set-exit-if-changed lib test integration_test
flutter analyze
flutter test
flutter test integration_test/windows_reader_workflow_test.dart -d windows
flutter build windows --release
```

Run applicable focused tests in addition to this common gate. The two opt-in
real-document audits require copied Arabic and English PDFs:

```powershell
$env:ATLAS_AUDIT_ARABIC_PDF = 'C:\path\to\arabic-copy.pdf'
$env:ATLAS_AUDIT_ENGLISH_PDF = 'C:\path\to\english-copy.pdf'
flutter test test/runtime_pdf_audit_test.dart
```

## Checkpoint ledger

| ID | Release | Checkpoint | Status | Owner approval |
|---|---|---|---|---|
| C0 | `0.8.0-beta.1` | Baseline acceptance | **Accepted** | PASS — 2026-09-09 |
| C1 | `0.8.0-beta.2` | Reader workspace | **Ready for owner test** | Pending |
| C2 | `0.8.0-beta.3` | Complete bookmarks | **Not started** | — |
| C3 | `0.8.0-beta.4` | Text markup and notes | **Not started** | — |
| C4 | `0.8.0-beta.5` | Advanced annotations | **Not started** | — |
| C5 | `0.8.0-beta.6` | Windows printing | **Not started** | — |
| C6 | `0.8.0-beta.7` | Covers and library details | **Not started** | — |
| C7 | `0.8.0-beta.8` | Portable metadata | **Not started** | — |
| C8 | `0.9.0-beta.1` | Arabic, RTL, and accessibility | **Not started** | — |
| C9 | `0.9.0-beta.2` | Performance and hardware qualification | **Not started** | — |
| C10 | `1.0.0-rc.1` → `1.0.0` | Installer and release candidate | **Not started** | — |

## C0 — Baseline acceptance

**Goal:** Establish the focused Library → Bookmark → PDF Ink workflow as the
accepted starting point for all later checkpoints.

**Implementation exit:** Documentation reports the current verified results,
and the release executable builds successfully. No feature work is included.

**Required fixtures:** A writable copy of a PDF with a nested outline, plus a
folder containing at least one Arabic-titled and one English-titled PDF.

**Owner test:**

1. Run `build\windows\x64\runner\Release\atlas_poc.exe`.
2. Use **Open PDF** on the copied PDF outside every registered library folder;
   confirm it appears in **Recents** without enrolling its parent folder.
3. Add the fixture folder, scan it, and confirm both Arabic and English books
   appear and can be filtered.
4. Use `Ctrl+K` to find a nested bookmark and confirm Atlas opens its exact page.
5. Enter **Write**, draw a mouse stroke, save, close, and reopen the document;
   confirm the stroke remains on its source page.
6. Confirm the original outline and metadata still exist and the sibling
   `.atlas-backup` recovery file was created by the save.

**Stop gate:** Record PASS and the approval date in the ledger, or record each
failure under the template below. Do not start C1 before explicit PASS.

## C1 — Reader workspace

**Goal:** Make the existing reader and workspace prototypes one dependable
desktop reading surface.

**Implementation exit:** The reader has a persistent, collapsible
**Outline/Pages/Bookmarks/Annotations** panel with search and selected-page
state; lazy real thumbnails; 10–6400% free zoom plus fit-page and fit-width;
Write-mode pan/zoom; equal library cards with generated PDF covers; accessible
book/tab context menus; and optional tab restoration (off by default).

**Owner test:** Follow the numbered corrective C1 checklist in this record.

**Stop gate:** All navigation paths land on the requested page, session state
restores, no control obscures content, and the workflow passes at 200% scale.

### Acceptance record — C1

- Status: Ready for owner test
- Commit: C1 reader-workspace candidate on `main`
- Version/build: `0.8.0-beta.2+4`
- Executable: `build\windows\x64\runner\Release\atlas_poc.exe`
- Automated commands and results: formatting clean; analysis 0 issues; 56
  unit/widget tests passed; 2 personal-file audits skipped; 3 Windows
  integration scenarios passed; release build succeeded
- Focused tests: 10% under-fit zoom, fit/custom transitions, Outline-first
  navigation, real page previews and PDF covers, fixed card geometry, lazy tab
  sessions, context actions, Arabic UI, and opt-in tab restoration
- Fixture copies required: three PDFs, including one mixed Arabic/English PDF
  with an outline and one writable PDF copy for Write-mode testing
- Known limitations: PDF is the only readable/writable format; the two
  personal-file audits remain unrun without owner-supplied fixture copies
- Hardware evidence supplied: desktop mouse/keyboard only; pen, touch, HDD,
  and physical printer remain required at C9
- Owner result: Pending corrective build retest
- Defects found: Build 3 lag, constrained zoom, missing covers, uneven cards,
  Pages-first navigation, and missing context actions were corrected in build 4
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
10. In Settings, verify tab restoration is off by default, then enable it and
    confirm tabs return after restarting Atlas.

## C2 — Complete bookmarks

**Goal:** Make PDF-outline bookmark management complete and interoperable.

**Implementation exit:** Users can create folders and bookmarks, edit, delete,
move, reorder, nest, and search them while Atlas preserves sibling order, full
hierarchical identity, page destinations, Unicode/Arabic titles, and the safe
PDF replacement contract.

**Owner test:** On a copied mixed Arabic/English PDF, create a three-level tree
with duplicate names under different parents; rename, reorder, move, nest, and
delete entries; save twice; reopen in Atlas and a standard PDF reader; confirm
titles, hierarchy, order, and destinations; verify the second sync reports zero
diff.

**Stop gate:** Both readers agree and the second sync is idempotent.

## C3 — Text markup and notes

**Goal:** Promote text markup and sticky-note prototypes into the focused reader
as reliable standard PDF annotations.

**Implementation exit:** Highlight, underline, strikeout, and sticky-note tools
support selection, editing, deletion, undo/redo, and page isolation, with
portable save/reopen behavior.

**Owner test:** Add every type on multiple pages, edit and delete samples, use
undo/redo, save, and reopen in Atlas and a standard reader. Confirm annotations
stay on the correct pages and untouched pages, outlines, metadata, and existing
annotations remain unchanged.

**Stop gate:** Every type round-trips in both readers without collateral loss.

## C4 — Advanced annotations

**Goal:** Complete editable drawing objects and annotation discovery.

**Implementation exit:** Free text, callouts, lines, arrows, rectangles,
ellipses/clouds, move, resize, style, and delete work through standard PDF
representations. The annotation panel lists, searches, filters, sorts, and
jumps to exact locations with keyboard and screen-reader access.

**Owner test:** Create and modify every type on multiple pages; search and
filter the panel; jump to each result; perform the workflow without a mouse;
save and reopen externally; confirm unrelated annotations survive.

**Stop gate:** All objects remain editable after reopen, panel jumps are exact,
and the complete workflow is keyboard accessible.

## C5 — Windows printing

**Goal:** Print locally through Windows with predictable layout and no document
mutation.

**Implementation exit:** Native printer selection and preview support all,
current, and custom ranges; page labels; odd/even and reverse order; copies and
collation; orientation and paper size; fit, actual size, custom scale, shrink,
multi-up, booklet, grayscale, and margins. The preview shows a clear summary.

**Owner test:** Hash a copied source PDF, exercise every setting through
Microsoft Print to PDF, inspect output page selection/order/layout, cancel one
job, and confirm the source hash never changes. Track a physical-printer run as
pending C9 hardware evidence.

**Stop gate:** Desktop test passes and no print or cancel path changes the PDF.

## C6 — Covers and library details

**Goal:** Give every discovered book a useful cover and accurate read-only
details.

**Implementation exit:** Atlas prefers a supported embedded cover/thumbnail,
renders PDF page one as the deterministic fallback, invalidates stale cache
entries, and displays complete bibliographic, technical, and reading facts with
portable and local-only values clearly distinguished.

**Owner test:** Scan PDFs with and without embedded thumbnails plus an EPUB;
inspect image accuracy and refresh behavior; then rename, move, modify, and
rescan the files and confirm covers and details follow the correct books.

**Stop gate:** Every readable PDF has a correct cover and no stale cover or
metadata is associated with a renamed or changed file.

## C7 — Portable metadata

**Goal:** Edit portable bibliographic and research metadata without damaging
unrelated PDF data.

**Implementation exit:** The metadata sheet supports field-level reset,
validation, before/after preview, and explicit **Save into PDF**. It maps core
fields to document information/XMP, preserves unknown namespaces, and labels
local-only fields. It uses the standard validated temporary-save and backup
path.

**Owner test:** On copied Arabic and English PDFs, edit portable fields, cancel
one attempt, save another, and reopen in Atlas and an external metadata
inspector. Verify outlines, annotations, unknown XMP, page content/count, and
backup recovery remain intact.

**Stop gate:** Both inspectors agree on portable fields and all preservation
checks pass.

## C8 — Arabic, RTL, and accessibility

**Goal:** Complete the Windows accessibility and Arabic release contract across
all work delivered through C7.

**Implementation exit:** All user-facing strings are localized; CI rejects new
hard-coded UI strings; locale-aware direction, bidi isolation, Arabic search
normalization and documented sorting, localized semantics/announcements,
visible focus, high contrast, Narrator, and 100%/200% scaling are covered by
automated and manual evidence. Earlier checkpoints must add English and Arabic
resources for each new UI rather than waiting for C8.

**Owner test:** In Arabic, complete library, reader, bookmark, annotation,
metadata, and print workflows using keyboard and Narrator at 100% and 200%
scale. Verify mixed Arabic/English/Urdu titles, filenames, paths, identifiers,
dates, page labels, and numbers remain readable and correctly ordered.

**Stop gate:** The published Arabic/RTL/accessibility matrix has no failed or
untested core workflow.

## C9 — Performance and hardware qualification

**Goal:** Prove Atlas meets its responsiveness, recovery, and Windows hardware
targets before release-candidate packaging.

**Implementation exit:** A reproducible benchmark fixture and results cover a
large library, a long document, 10,000 bookmarks, interrupted writes, mouse,
touch, pen, SSD, HDD, and physical printing. Test output has no unexplained
warnings, and regressions have enforceable thresholds where practical.

**Owner test:** Run the published matrix on representative hardware; confirm
10,000-bookmark search is below 100 ms p95, progressive scans keep the UI
responsive, interrupted saves recover without corruption, pen/touch strokes are
page-isolated, and a physical print matches its preview.

**Stop gate:** Block C10 and stable 1.0 while any required hardware evidence is
missing or any threshold fails.

## C10 — Installer and release candidate

**Goal:** Ship an owner-approved, reproducible Windows 1.0 artifact.

**Implementation exit:** An unsigned Windows installer supports clean install,
upgrade, uninstall, and optional user-controlled `.pdf` association. It includes
licenses, release notes, backup/recovery guidance, SmartScreen expectations,
and CI-produced artifacts. Installation and uninstall never remove user books.

**Owner test:** Test a clean install and an upgrade from the accepted baseline;
launch normally and through **Open with**; decline and then enable association;
confirm the database and PDFs persist; uninstall and verify user books remain;
reinstall and run the complete regression checklist.

**Stop gate:** Publish `1.0.0-rc.1`, obtain explicit owner approval, and promote
the identical accepted artifact to `1.0.0`. Any code or packaging change after
approval requires a new release candidate and regression pass.

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

Windows 11 is the primary 1.0 platform. The roadmap promises no dates. TTS,
speed-reading in the primary UI, cloud accounts/synchronization, EPUB rendering
or ink, OCR, forms, signatures, conversion, hosted sharing, and multimedia
remain explicitly deferred.
