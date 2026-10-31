# Atlas Reader — Checkpoints to Windows 2.0

<!-- atlas-status: N4|in-progress -->

This file owns **order and status**. Direction and scope are in [PLAN.md](PLAN.md); test methods and speed budgets are in [docs/QUALITY.md](docs/QUALITY.md).

## Status now

| | |
|---|---|
| Active stage | **N4: Reader**, planned `2.0.0-beta.2` |
| Active sub-stage | **N4.3: Navigation** (next). N4.2 Virtual viewport is implemented and verified: pages display (CI `41242163280`, 45/45) |
| Done in N4 | N4.1 Opening, tabs and passwords: implemented and automated-verified (43/43 CI tests, run `36690884159`) |
| Branch | `native-v2-n4-reader-foundation`, from accepted `main` merge `739508e3` |
| Released | `2.0.0-beta.1` (Library), 2026-09-28 |
| Next owner review | End of N4, with one review build and one short guide ([N4 review](docs/N4_REVIEW.md)) |
| Evidence | [N4 evidence ledger](docs/baselines/N4_READER_FOUNDATION_EVIDENCE.md) |

## Roadmap

| ID | Stage | Version | Status |
|---|---|---|---|
| N0 | Repository bootstrap | `2.0.0-alpha.0` | **Accepted** 2026-09-22 |
| N1 | Windows toolchain + app shell | `2.0.0-alpha.1` | **Accepted** 2026-09-22 |
| N2 | PDF engine qualification | `2.0.0-alpha.2` | **Accepted** 2026-09-27 |
| N3 | Library and index | `2.0.0-beta.1` | **Accepted; Released** 2026-09-28 |
| N4 | Reader | `2.0.0-beta.2` | **In progress** |
| N5 | Bookmarks + document safety | `2.0.0-beta.3` | Not started |
| N6 | Ink + annotations | `2.0.0-beta.4` | Not started |
| N7 | Everyday tools | `2.0.0-beta.5` | Not started |
| N8 | Backup, restore + migration | `2.0.0-beta.6` | Not started |
| N9 | Arabic/RTL + accessibility qualification | `2.0.0-beta.7` | Not started |
| N10 | Speed, hardware + release candidate | `2.0.0-rc.1` | Not started |
| N11 | Stable Windows 2.0 | `2.0.0` | Not started |

After 2.0: formats, editing tools and platforms in [docs/AFTER_2_0.md](docs/AFTER_2_0.md). Versions after beta.1 were renumbered on 2026-11-01 (focused 2.0); nothing after beta.1 had been published. A corrective build uses `+N` build metadata or the next beta number, and a version is never reused for different bytes.

## How a stage runs

This replaces the earlier rule that every sub-stage waited for an owner sign-off.

1. **Build in order.** Sub-stages go in sequence. Each one closes on **automated evidence**: tests at the exact branch head, Debug and Release CI green, plus a rendered self-review (screenshots in English/Arabic, light/dark, 100%/200%, wide/narrow) recorded in the evidence ledger.
2. **Ask early when design needs a decision.** When a visual direction is uncertain, the owner gets a short screenshot or video check-in. This is optional and never blocks progress.
3. **One owner review per stage.** At the end of the stage the owner receives one checksummed review build and one plain-language guide: at most about 10 checks, about 45 minutes, and every skip stated.
4. **Fix before moving on.** Each defect from the review gets a fix plus a regression test or numbered manual check. Only the failed items are rechecked.
5. **Next stage may start** once the review build is delivered and all automated gates are green. Fixing review defects takes priority over new work. **Merging to `main`, tagging and publishing the beta wait for the owner's explicit `N# PASS`.**

### A stage is Accepted only when

1. Its scope is implemented and its exclusions were respected.
2. All required automated checks pass at the exact head.
3. Every changed screen was reviewed rendered, in populated, empty, error, narrow, 200%, Arabic/RTL, light and dark states. Approving from code or one screenshot is not enough.
4. Every known limitation is fixed or moved to a named later stage with a blocking test. "Later" on its own is not allowed.
5. Shared models (identity, Library, metadata, search, progress, backup) gained no unnecessary PDF-only assumptions.
6. The owner has written the exact words **`N# PASS`**.

A passed stage is not reopened without new evidence of a user-facing regression.

---

## N0–N3: Accepted foundation

| Stage | Result | Record |
|---|---|---|
| N0 Bootstrap | Architecture, scope and build path | [handoff](docs/history/plan-v1/docs/N0_HANDOFF.md) |
| N1 Windows shell | VS 2022 / MSVC v143 / Qt 6.10.3 baseline; shell performance, Arabic/RTL, theme, scale and keyboard qualified; runtime `73f567cf` | [N1 qualification](docs/baselines/N1_FINAL_QUALIFICATION.md) |
| N2 PDF engines | Qt PDF 6.10.3 reads/renders; qpdf 12.4.1 CLI for structure/security/writes; PDFium kept for comparison only | [ADR-0004](docs/decisions/ADR-0004-pdf-engine-responsibilities.md) · [matrix](docs/baselines/N2_PDF_ENGINE_MATRIX.md) |
| N3 Library | SQLite/FTS5 index, multi-folder scanning, guarded identity, Favorites/Recents/search; Calibre 9.9 comparison | [N3 acceptance](docs/baselines/N3_ACCEPTANCE.md) |

---

## N4 — Reader (`2.0.0-beta.2`)

**Goal:** comfortable daily PDF reading inside Atlas: accurate, fast, bounded in memory and beautiful. N4 is **read-only**: source PDFs must stay byte-identical.

**Excluded:** bookmark editing, annotations, any PDF write, side-by-side and reflow view (after 2.0), installer.

### N4.1 — Opening, tabs and passwords ✅ Implemented and verified

Background open from the Library or a direct path; one tab per file; tab switching and closing; clear missing, malformed, password and unsupported states; password retry; opt-in, path-only restore; no file handle left locking the PDF. CI `36690884159` at `5ac63df`: 43/43 Debug and Release. The rendered owner check moves into the end-of-N4 review.

### N4.2 — Virtual viewport ✅ Implemented and verified (CI `41242163280`, 45/45)

- Engine-independent page geometry and render requests; visible pages render first, then neighbors; obsolete work is cancelled.
- Scale buckets, texture ownership, cache keys, eviction and hard CPU/GPU memory budgets. Never render the whole document up front.
- While zooming or scrolling fast, show the existing texture and sharpen asynchronously; never block input.
- **Tests:** mixed sizes, all rotations, crop/media boxes, fast jumps, resize and scale changes, cancellation, cache invalidation, a 2,000-page memory series (memory must not grow with page count).

### N4.3 — Navigation ← **next**

- Continuous and single-page; facing and cover spreads; reading direction set separately from UI language (C03).
- Zoom, actual size, fit page/width/height; page jump by number or label; academic offset; thumbnails.
- Back/Forward history across links, jumps and search results; per-book position and zoom remembered; focus/full-screen mode with a discoverable exit (C02).
- **Tests:** Fit/FitH/FitV normalization, bounds, labels, rotations, spread rules, LTR/RTL progression, focus at 100% and 200%.

### N4.4 — Outline, text, search and links

- Read-only outline; text selection and copy; cancellable search with next/previous; internal and external links through Atlas-owned models.
- Semantic de-duplication of Qt PDF's duplicate link rows; destinations normalized with rotation and crop.
- Document text and reading order exposed to assistive technology; tagged, untagged and image-only content are told apart (C04).
- No document JavaScript, launches or remote fetches; external links show their destination and need a click to confirm (C05, C22). Honest unsupported states for forms, XFA, attachments and signatures (C15).
- **Tests:** link-heavy fixtures, rotated destinations, Arabic/Urdu/English and mixed-direction search, p50/p95/p99 search latency on long books, image-only honesty, hostile-action fixtures with the network blocked.

### N4.5 — Reading workspace and design system

- **Design-system foundation** ([DESIGN.md](docs/DESIGN.md)): a `Theme` singleton with tokens; shared components (button, icon button, text field, tab strip, side panel, tree, banner, empty state, dialog). Replace hard-coded colors, sizes and inline `arabic ? … : …` strings with tokens and Qt Linguist translations (`qsTr` + `.ts` for English and Arabic). `Main.qml` gets split into focused views.
- Compose the workspace: canvas with gutters, tab strip, collapsible navigation panel (Outline · Pages), search results, status and errors, and a final shortcut map (resolve `Ctrl+O` Library vs Reader).
- **Tests:** rendered review in English/Arabic × light/dark/high contrast × wide/narrow × 100%/200%; long names and overflow; keyboard-only route; Narrator names and roles; no literal colors outside `Theme`.

### Engineering speed tasks in N4

- Limit the seven inherited N2 PDF regression workflows to run when PDF adapter paths change, plus a nightly schedule and manual runs, so ordinary pushes get fast feedback. Windows CI stays on every push.
- Add a release-build benchmark target for first page, page jump, scroll and search (see [QUALITY.md](docs/QUALITY.md)).

### Automated QA

- Open, error, session and opt-in restore contracts (done in N4.1).
- Viewport, cache, eviction, priority and cancellation; page labels, boxes and all four rotations.
- Link de-duplication and exact rotated destinations; text/search correctness and cancellation; search p50/p95/p99.
- 2,000-page, multi-tab and repeated open/close memory series.
- Keyboard, focus and UI Automation assertions; Arabic/Urdu fixtures; source SHA-256 unchanged after every test.

### Owner review

Using the [N4 review guide](docs/N4_REVIEW.md): open an image-only, an Arabic/Urdu, a 1,000+ page, a mixed-size/rotated and a link-heavy PDF. Try tabs, zoom/fit, page labels, search, links, outline, thumbnails, spreads, reading direction, Back/Forward and restore, with mouse and keyboard. Check Narrator, 200%, Arabic and dark mode. Run the same tasks in Foxit or SumatraPDF and note what feels better or worse. Includes the N4.1 checks (passwords, file not locked).

### Stop gate

Owner records **`N4 PASS`** after required fixes, when the reader is comfortable for daily reading, all blocking QA is green and the reviewed build is the one that gets merged. Wrong navigation, broken Arabic shaping or direction, unbounded memory, repeated UI stalls or an inaccessible core path block it.

---

## N5 — Bookmarks + document safety (`2.0.0-beta.3`)

**Goal:** make deep research navigation Atlas's signature, safe on every kind of PDF. Contract: [DATA_AND_SAFETY.md](docs/DATA_AND_SAFETY.md).

- **N5.1 Capability check:** open vs edit permission; open password vs permissions password; read-only file system; signed/certified; locked; offline; external revision. Modern encryption fixtures; never a bypass.
- **N5.2 Local overlay:** Embedded/Local/Pending/Override/Hidden/Conflict states persisted and recoverable after a crash. Versioned location envelope for durable destinations (C06).
- **N5.3 Complete editor:** create, rename, delete, edit destination, move, reorder, nest; duplicate names; deep Unicode trees; keyboard tree semantics; undo. Bookmarks tab in the navigation panel; **Command Center** (`Ctrl+K`) for books and bookmarks.
- **N5.4 Reconcile and safe save:** diff, merge, conflicts; temp → validate → backup → replace → reopen; never a blind overwrite.
- **N5.5 Portability:** Atlas JSON import/export (versioned, rejects newer schemas), Markdown, CSV; all, subtree or local-only.

### Automated QA

- Capability, password, restriction, signed and certified classification.
- Overlay persistence and crash/reopen transitions; create/rename/delete/move/reorder/nest with duplicate Unicode names.
- 10,000-node search and navigation under 100 ms p95; keyboard tree semantics.
- External-revision conflicts; safe-save failure injection (lock, permission loss, disk full, crash at each step).
- JSON/Markdown/CSV round trips; independent-reader reopen after every writable mutation; preservation of unrelated objects.

### Owner review

Run each fixture through edit → close → reopen → conflict → recovery: normal, restricted, read-only, locked, signed, externally changed, offline, Arabic/English deep tree, duplicate names, 10k nodes. Confirm protected originals are untouched, local work is clearly labeled local, and embedded results match in another reader.

### Stop gate

**`N5 PASS`**, with no research loss, no restriction bypass, no blind overwrite and no false "embedded" in any tested state.

---

## N6 — Ink + annotations (`2.0.0-beta.4`)

**Goal:** writing that feels direct and saves as standard PDF annotations where permitted.

- Live GPU stroke layer, never coupled to PDF saving; pen and highlighter presets; pressure where verified; eraser (whole and partial); lasso/rectangle selection; move, resize, copy, duplicate, delete; undo/redo (C07).
- Text highlight, underline, strikeout, sticky and typed notes, line/arrow/rectangle/ellipse (C19); annotation list with search, filter and jump (C08); show/hide without deleting (C16).
- Annotation JSON/Markdown export; explicit print/flatten-to-new-copy choices; reuses N5 safety for protected files.
- Published table of annotation types and operations, including imported read-only types.

### Automated QA

- Coordinate normalization on rotated, cropped and zoomed pages; page isolation; gesture separation (pan never draws).
- Undo chains; overlay persistence and crash recovery within a declared loss budget.
- Serialization and preservation; independent-reader rendering corpus; JSON round trip; source unchanged on flatten-to-copy.

### Owner review

On real pen/touch hardware, draw short, long and fast strokes while scrolling, zooming and indexing. Annotate Arabic text; reopen normal and protected files in Atlas and another reader; check latency, fidelity and fallback.

### Stop gate

**`N6 PASS`**: low latency, independent-reader round trip, no damage to other content in the PDF.

---

## N7 — Everyday tools (`2.0.0-beta.5`)

**Goal:** the essential desktop utilities, without turning Atlas into an office suite.

- Printing with preview, ranges, labels, layout and grayscale; source hash unchanged.
- Covers queue with invalidation; details panel; selected metadata/XMP editing with preview and preservation.
- Sorting, filters, safe rename, collections and tags (C09, C10, C21).
- Open With, drag and drop, clipboard, long and Unicode paths, session-end safety; settings; privacy-safe logs and diagnostic bundle.

### Automated QA

- Cover cancellation and cache bounds; print range model and hash invariance; metadata preservation fixtures.
- Sorting with ties, missing metadata, 10k entries and RTL; rename collision and recovery; collections that survive rescans and moves.
- Launch, Open With, drag and drop, clipboard and long-path behavior; settings defaults and reset; log redaction.
- Full Index/Reader/Bookmarks regression suite.

### Owner review

Print to a real printer, edit permitted metadata, rename a book, build a collection, open through Windows entry points and drag-and-drop, review settings, and preview a diagnostic bundle. Confirm the source hash is unchanged and logs are private.

### Stop gate

**`N7 PASS`**, with no regression in Library, Reader or Bookmarks.

---

## N8 — Backup, restore + migration (`2.0.0-beta.6`)

**Goal:** existing users can move over safely and restore their research on a clean profile.

- Versioned backup package; consistent snapshot; restore with preview and unresolved books (C12).
- Read-only, idempotent import from the Flutter profile; side-by-side use and rollback; never relink by filename.

### Automated QA

- Manifest/schema validation and rejection of newer versions; clean-profile restore; interrupted backup; insufficient space.
- Legacy import repeated twice; ambiguous identity; corrupted or partial input; legacy and source hashes unchanged.

### Owner review

Using a copy of real legacy data: preview matches, import twice, run old and new apps side by side, restore the backup to a clean profile. Confirm nothing in the legacy profile or the PDFs changed.

### Stop gate

**`N8 PASS`**, after clean-profile restore and representative migration pass.

---

## N9 — Arabic/RTL + accessibility qualification (`2.0.0-beta.7`)

**Goal:** close the remaining gaps across everything built. This is qualification, not the first time these are done.

Run the full workflow matrix (Library, Reader, Bookmarks, protected/local, conflicts, import/export, ink, print, metadata, backup/migration) in Arabic UI with mixed Arabic/English/Urdu data, keyboard-only, with Narrator, Accessibility Insights, 100%/200%, high contrast and reduced motion (C13).

### Automated QA

- Translation completeness with no English fallback in core screens.
- Bidi isolation, exact Unicode round trips, normalization, search and collation.
- Keyboard traversal and UI Automation role/name/state assertions; scale and high-contrast checks; a coverage report with no blank core cell.

### Owner review

Repeat the matrix above personally, focusing on Arabic reading comfort and Narrator flow.

### Stop gate

**`N9 PASS`**, with no core workflow untested or failing in the published matrix.

---

## N10 — Speed, hardware + release candidate (`2.0.0-rc.1`)

**Goal:** prove the feature-complete build on real hardware and package it.

Large libraries; long PDFs; 10k+ bookmark trees; SSD and HDD/external drives; pen and touch; multiple DPIs and monitors; a physical printer; interrupted saves, conflicts and offline folders; long sessions; install/upgrade candidate; SBOM, notices and security review; network-observed offline run (C05, C14, C22).

### Automated QA

- Full Debug/Release regression; benchmark thresholds; long memory/handle soak.
- Fuzzing of Atlas-owned parsers (bookmark JSON, backup manifest, migration input); failure injection.
- Clean package, install and upgrade dry runs; reproducibility; dependency vulnerability, license, notice, SBOM and secrets review.

### Owner review

Run the hardware matrix on named machines and compare core tasks against mature readers on the same files. Every skipped item is recorded; a skipped required item fails the review.

### Stop gate

**`N10 PASS`**. Missing required hardware evidence or a failed P0 budget blocks stable.

---

## N11 — Stable Windows 2.0 (`2.0.0`)

**Goal:** promote the accepted release candidate without changing it.

Installer technology chosen and qualified here; clean install/upgrade/uninstall; optional, user-controlled `.pdf` association; uninstall never deletes books, research or backups; release notes with known limitations; source, tag, artifact, checksum, SBOM and notices archived.

### Automated QA

- Payload equivalence between the accepted RC and stable; install/upgrade/uninstall matrix; checksum, tag, notices and SBOM verification; final smoke test from the distributable.

### Owner review

Install, upgrade, use and uninstall the exact candidate on a clean profile; confirm file association choice and preserved data.

### Stop gate

**`N11 PASS`** before tagging `v2.0.0`. No payload drift, missing evidence or user-data deletion.

---

## Inherited limitations with named owners

| From N2 | Fixed in | Blocking evidence |
|---|---|---|
| Qt PDF returns duplicate raw link rows | N4.4 | De-duplication test plus link-heavy fixture |
| Rotated explicit destinations need rotation metadata | N4.4 | Exact targets on normal, cropped, 90°, 180° and 270° pages |
| Qt PDF search slower than PDFium on synthetic tests | N4.4 | Long-book p50/p95/p99, cancellation, no missed results |
| Qt PDF memory trend needs real-reader proof | N4.2, N10 | 2,000-page and multi-tab memory series |
| Fit/FitH/FitV handling | N4.3 | Destination-mode fixtures |
| Broader graphics/annotation rendering corpus | N4, N6 | Image, gradient, transparency and annotation subtypes |
| Security-handler breadth | N5.1 | Modern and unsupported encryption fixtures, no bypass |
| Signature presence ≠ validity | N5.1 (detect only); validation after 2.0 | Signed originals preserved; never a "valid" claim |
| One corrected Arabic/Urdu page is not a corpus | N4, N9 | Several real-font Arabic/Urdu PDFs: joining, diacritics, search, direction |

## Competitive action IDs (C01–C22)

Kept for traceability with the [2026-09-27 audit](docs/history/plan-v1/docs/COMPETITIVE_GAP_ACTIONS.md). In 2.0: C01 (N3, done) · C02–C05, C15, C22 (N4) · C06 (N5) · C07, C08, C16, C19 (N6) · C09, C10, C21 (N7) · C12 (N8) · C13 (N9) · C14 (N10–N11). **After 2.0:** C11 read-aloud, C17 side-by-side, C18 reflow view, C20 form filling; see [AFTER_2_0.md](docs/AFTER_2_0.md).
