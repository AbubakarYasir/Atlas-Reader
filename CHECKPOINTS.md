# Atlas Reader — Checkpoints to Windows 2.0

<!-- atlas-status: N5|in-progress -->

This file owns **order and status**. Direction and scope are in [PLAN.md](PLAN.md); test methods and speed budgets are in [docs/QUALITY.md](docs/QUALITY.md).

## Status now

| | |
|---|---|
| Active stage | **N5: Bookmarks + document safety**, planned `2.0.0-beta.3` |
| Active sub-stage | **N5.4: Reconcile and safe save** (N5.1–N5.3 implemented) |
| Done | N4 Reader **Accepted** (`N4 PASS`, 2026-11-02; [acceptance](docs/baselines/N4_ACCEPTANCE.md)) |
| Branch | `native-v2-n5-bookmarks-safety`, from `main` after the `2.0.0-beta.2` release |
| Released | `2.0.0-beta.2` (Reader), 2026-11-02 · `2.0.0-beta.1` (Library), 2026-09-28 |
| Next owner review | End of N5, with one review build and one short guide |
| Evidence | [N5 evidence ledger](docs/baselines/N5_BOOKMARKS_SAFETY_EVIDENCE.md) |

## Roadmap

| ID | Stage | Version | Status |
|---|---|---|---|
| N0 | Repository bootstrap | `2.0.0-alpha.0` | **Accepted** 2026-09-22 |
| N1 | Windows toolchain + app shell | `2.0.0-alpha.1` | **Accepted** 2026-09-22 |
| N2 | PDF engine qualification | `2.0.0-alpha.2` | **Accepted** 2026-09-27 |
| N3 | Library and index | `2.0.0-beta.1` | **Accepted; Released** 2026-09-28 |
| N4 | Reader | `2.0.0-beta.2` | **Accepted; Released** 2026-11-02 |
| N5 | Bookmarks + document safety | `2.0.0-beta.3` | **In progress** |
| N6 | Ink + annotations | `2.0.0-beta.4` | Not started |
| N7 | Everyday tools | `2.0.0-beta.5` | Not started |
| N8 | Backup, restore + migration | `2.0.0-beta.6` | Not started |
| N9 | English/Arabic/Urdu + accessibility qualification | `2.0.0-beta.7` | Not started |
| N10 | Speed, hardware + release candidate | `2.0.0-rc.1` | Not started |
| N11 | Stable Windows 2.0 | `2.0.0` | Not started |

After 2.0: every remaining Foxit feature, formats and platforms in tracks A–K of [docs/AFTER_2_0.md](docs/AFTER_2_0.md) (full map in [FOXIT_PARITY.md §3](docs/FOXIT_PARITY.md#3-complete-foxit-feature-catalogue-and-atlas-plan)). Versions after beta.1 were renumbered on 2026-11-01 (focused 2.0); nothing after beta.1 had been published. A corrective build uses `+N` build metadata or the next beta number, and a version is never reused for different bytes.

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

## N4 — Reader (`2.0.0-beta.2`) ✅ Accepted 2026-11-02 · Released

Owner `N4 PASS` on 2026-11-02 ([acceptance](docs/baselines/N4_ACCEPTANCE.md)); open items carried forward are listed there.

**Goal:** comfortable daily PDF reading inside Atlas: accurate, fast, bounded in memory and beautiful. N4 is **read-only**: source PDFs must stay byte-identical.

**Excluded:** bookmark editing, annotations, any PDF write, side-by-side and reflow view (after 2.0), installer.

### N4.1 — Opening, tabs and passwords ✅ Implemented and verified

Background open from the Library or a direct path; one tab per file; tab switching and closing; clear missing, malformed, password and unsupported states; password retry; opt-in, path-only restore; no file handle left locking the PDF. CI `36690884159` at `5ac63df`: 43/43 Debug and Release. The rendered owner check moves into the end-of-N4 review.

### N4.2 — Virtual viewport ✅ Implemented and verified (CI `41242163280`, 45/45)

- Engine-independent page geometry and render requests; visible pages render first, then neighbors; obsolete work is cancelled.
- Scale buckets, texture ownership, cache keys, eviction and hard CPU/GPU memory budgets. Never render the whole document up front.
- While zooming or scrolling fast, show the existing texture and sharpen asynchronously; never block input.
- **Tests:** mixed sizes, all rotations, crop/media boxes, fast jumps, resize and scale changes, cancellation, cache invalidation, a 2,000-page memory series (memory must not grow with page count).

### N4.3 — Foxit workspace, design system and three languages ✅ Implemented and verified (CI `41281081373`)

Moved forward on 2026-11-01 so every later reader feature lands in the final workspace. **Layout, measurements and shortcuts follow Foxit PDF Editor** ([ADR-0007](docs/decisions/ADR-0007-foxit-workspace-parity.md), measured spec in [FOXIT_PARITY.md](docs/FOXIT_PARITY.md)); icons, colours and name are Atlas's.

- **Done in the first pass (commit `2cc4a97`, CI 41277932148 green):** `Theme`/`I18n`/`AppState` singletons, shared components, complete English/Arabic/Urdu tables with live switching and CI completeness check, bundled Nastaliq/Naskh, reading colours, rotation, Read mode, Full screen, Light/Dark/System, Touch mode, library list/grid, files opened from the command line.
- **Foxit workspace (second pass):** custom 48 px title bar (app mark, Quick Access Toolbar with Open and Mouse/Touch mode, file name, window buttons with Windows snap) · ribbon tab row (**File, Home, View, Accessibility, Help**; others appear with their stage) with 72 px grouped command area, large/small buttons, collapse (Ctrl+F11) · 28 px document tab bar with fixed 124 px tabs and **Library as the first tab** · 40 px navigation rail (F4; panels arrive with their features) · status bar (« ‹ page/total › », zoom −/slider/+/percentage, full screen) · **File page** (Open: Recent / Computer, Close, Close all, Skins, Preferences, About, Exit) · **Preferences window** (searchable categories: General, Languages, Skins, Page Display, Accessibility) · **Skins** (Light/Dark/System; Classic or Colourful light theme with Atlas colours).
- **Home tab now:** Hand · Zoom ▾ (in, out, levels) · Page Fit Option ▾ (Actual Size, Fit Page, Fit Width) · Rotate View ▾. **View tab now:** Read Mode · Visual Mode ▾ (reading colours) · Rotate View · View Setting ▾ (Navigation Panels, Status Bar, Full Screen). **Accessibility:** Reading Option (reading colours, Touch mode). **Help:** About Atlas Reader.
- **Keyboard:** Foxit defaults (Ctrl+K Preferences, Ctrl+Q exit, Ctrl+Shift+W close all, Ctrl+F11 ribbon, F4 panels, Ctrl+H, F11, Ctrl+1/2/0, Ctrl+Shift+±, Ctrl+Home/End, Ctrl+G); Add folder moves to Ctrl+Shift+O; Ctrl+F finds in the Library.
- **Tests:** string-table completeness (EN/AR/UR); rendered screenshots in English, Arabic and Urdu × Light/Dark compared side by side with Foxit at 1280 × 800 and 1920 × 1040; keyboard route and accessible names for every ribbon control; existing viewport and controller suites.

### N4.4 — Navigation ✅ Implemented (automated checks; owner review at end of N4)

- **Done (slice 3):** Fit Visible (Ctrl+3), Zoom To (Ctrl+M), Marquee Zoom, Foxit's full zoom list (8.33 %–6400 %); AutoScroll (Ctrl+Shift+H; Up/Down speed, Esc or a click stops); page box accepts printed page labels ("iv", "A-3") and shows "label (n / total)"; **per-book view memory** (page, zoom/fit, layout) with Preferences → Documents; default layout and zoom (Preferences → Page Display); Preferences → History (Clear History keeps pinned files); recent-file menu (Open, Pin/Unpin, Copy Path, Open Containing Folder, Remove; pinned files listed first and kept past the 20-file limit); Reset All Panels; page-background context menu.
- **Moved, with owners:** Open a Copy (writes a file) and Preferences → Full Screen and rendering options → N7; tiling for pages wider or taller than 8,192 px → N10 (today such pages render at a capped, softer resolution, never crash); Fit Height is covered by Fit Page and Fit Visible, as in Foxit; an "academic offset" is replaced by printed page labels.

- **Done (slice 2):** **Page Display** — Single Page, Continuous, Facing, Continuous Facing, Show Cover Page During Facing (View tab small buttons and status-bar buttons, as in Foxit); **Reverse View** (spreads read right to left; Left arrow turns forward); next/previous page turn a whole spread; Single Page shows one page or spread and turns at its end.
- **Done (slice 1):** navigation rail with the **Pages panel** (background thumbnails, current page outlined, click or Enter to jump, resizable splitter, double-click hides, F4, View → View Setting → Navigation Panels); **Previous/Next View** (Alt+Left/Right and status-bar buttons; page-box, first/last, Home/End and thumbnail jumps are recorded, next/previous page is not); **document-tab context menu** (Close, Close Others, Close Left/Right, Close All, Copy Path, Open Containing Folder).

- Continuous and single-page; facing and cover spreads (View tab page-display buttons and status-bar layout buttons, as in Foxit); **Reverse View** = reading direction set separately from UI language (C03); **AutoScroll** (Ctrl+Shift+H, Esc stops).
- Fit height and **Fit Visible** (Ctrl+3); **Zoom to** (Ctrl+M) and marquee zoom; page jump by label; academic offset; **Pages panel** (thumbnails) in the rail; default layout/zoom preferences; tiling for pages beyond 8192 px.
- Back/Forward history across links, jumps and search results (Alt+Left/Right, status-bar buttons); per-book position and zoom remembered (C02).
- **Foxit workspace items:** navigation rail with Show/Reset panels; full zoom list (1 %–6400 %, Zoom To); page-background context menu; document-tab context menu (Close, Close Others/Left/Right, Copy Path, Open Containing Folder); recent-file menu (Open, Open a Copy, Pin, Copy Path, Remove); Preferences → Page Display, Full Screen, History, Documents.
- **Tests:** Fit/FitH/FitV normalization, bounds, labels, rotations, spread rules, LTR/RTL progression, focus at 100% and 200%.

### N4.5 — Outline, text, search and links ✅ Implemented (automated checks; owner review at end of N4)

- **Done:** **Bookmarks panel** (document outline, read-only; expand/collapse, current section highlighted, keyboard); **Find bar** (Ctrl+F, Enter/F3 next, Shift+Enter/Shift+F3 previous, live count) and **Search panel** (Ctrl+Shift+F, right side; results stream page by page with context; options shared and remembered); search runs on a background thread, starts at the current page, is cancelled by a newer search and highlights every hit; Arabic/Urdu matching ignores harakat, tatweel and joiners, folds letter variants **and shaped presentation forms** (FB50–FDFF, FE70–FEFF, lam-alef ligatures); **Select Text** (Alt+6), **SnapShot** (Alt+7, copies an area as an image), Hand (Alt+3), Clipboard ▾ (Copy Ctrl+C, Select All Ctrl+A, Deselect Ctrl+Shift+A); copy gives plain letters (NFKC); **Word Count** (View tab); **links** — internal links jump (and Back returns), web/mail links show their address and open only after confirmation (other schemes refused), right-click copies the link; garbage or /Fit destinations fall back to the top of the page; de-duplicated link rows (Qt 6.6+); "no searchable text" banner for image-only books; Preferences → Search; the current page's text is exposed to screen readers (C04).
- **Moved, with owners:** include bookmarks/comments in search → N6 (comments arrive there); image context menu (copy, save image as), Destinations panel, open PDF links in a new tab, single-key accelerators, Preferences → Reading (reading order, caret browsing) → N7; untagged-document prompt and tagged reading order → N10; search latency p50/p95/p99 on long books → measured on Windows in the N4 review build (Release), budget in [QUALITY.md](docs/QUALITY.md).

- **Outline panel** (read-only) and **Search panel** (right panel, Ctrl+Shift+F) with **Find** (Ctrl+F, F3/Shift+F3); Home → **Select** (Alt+6) and **SnapShot** (Alt+7), Clipboard ▾; View → **Word Count**; search options (whole words, case, include bookmarks/comments, highlight all); link menu (open after confirmation, copy link location); image menu (copy, save image as); destinations (go to); Preferences → Search, Reading; single-key accelerators option; text selection and copy; caret browsing and system selection colour options; cancellable search with next/previous; internal and external links through Atlas-owned models.
- Semantic de-duplication of Qt PDF's duplicate link rows; destinations normalized with rotation and crop.
- Document text and reading order exposed to assistive technology; tagged, untagged and image-only content are told apart (C04).
- No document JavaScript, launches or remote fetches; external links show their destination and need a click to confirm (C05, C22). Honest unsupported states for forms, XFA, attachments and signatures (C15).
- Search accepts any script; Arabic/Urdu normalization (letter variants, diacritics, tatweel) is documented and tested.
- **Tests:** link-heavy fixtures, rotated destinations, Arabic/Urdu/English and mixed-direction search, p50/p95/p99 search latency on long books, image-only honesty, hostile-action fixtures with the network blocked.

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

Using the [N4 review guide](docs/N4_REVIEW.md): open an image-only, an Arabic/Urdu, a 1,000+ page, a mixed-size/rotated and a link-heavy PDF. Try tabs, zoom/fit, page labels, search, links, outline, thumbnails, spreads, reading direction, Back/Forward and restore, with mouse and keyboard. Check the interface in English, Arabic and Urdu, plus Narrator, 200%, dark mode and reading colours. Run the same tasks in Foxit or SumatraPDF and note what feels better or worse. Includes the N4.1 checks (passwords, file not locked).

### Stop gate

Owner records **`N4 PASS`** after required fixes, when the reader is comfortable for daily reading, all blocking QA is green and the reviewed build is the one that gets merged. Wrong navigation, a missing or wrong translation, broken Arabic/Urdu shaping or direction, unbounded memory, repeated UI stalls or an inaccessible core path block it.

---

## N5 — Bookmarks + document safety (`2.0.0-beta.3`) ← **active**

**Goal:** make deep research navigation Atlas's signature, safe on every kind of PDF. Contract: [DATA_AND_SAFETY.md](docs/DATA_AND_SAFETY.md).

- **N5.1 done (2026-11-02, automated checks):** core `CapabilityPolicy` turns file, PDF-security and revision facts into what Atlas may change and why (every row of the [capability table](docs/DATA_AND_SAFETY.md#document-capabilities), tested); qpdf adapter (bundled 12.4.1 CLI, password through standard input) reads encryption, user/owner password, permission flags, signed fields and DocMDP certification; file probe reads read-only file/folder, sharing locks and cloud placeholders without writing anything; size and time recorded at open detect changes made outside Atlas. Reader status bar shows a quiet chip (Protected, Read-only, In use, Signed, Changed outside Atlas) with the reasons in plain words and *Check again*; re-checked when you return to a tab. Real-fixture tests: A001 plain, A009 wrong/user/owner password, A010 certification, Unicode-named copy, external change; sources byte-identical.
- **N5.2 done (2026-11-03, automated checks):** core overlay model with the six storage states and one explicit transition table; versioned location envelope (`atlas-location/1;scheme=pdf;…`, normalized page coordinates, unknown keys kept, newer versions kept unchanged); profile schema 5 with base snapshot, nodes, tags and tombstones ([ADR-0008](docs/decisions/ADR-0008-bookmark-overlay-and-location-envelope.md)); repository with one transaction per change, revision check and "no false Embedded" (an Embedded node must equal the PDF's node; local work reaches Embedded only through N5.4's verified save). Tests: every state × event pair, envelope round trips and rejections, exact Unicode (Arabic, Urdu harakat and joiners, emoji, U+0000, spaces), tombstones, schema-4 upgrade, 10,000 nodes, and crash recovery by ending a child process inside and right after a transaction. **Not yet visible in the app:** the Bookmarks panel reads the overlay in N5.3.
- **N5.1 Capability check:** open vs edit permission; open password vs permissions password; read-only file system; signed/certified; locked; offline; external revision. Modern encryption fixtures; never a bypass.
- **N5.2 Local overlay:** Embedded/Local/Pending/Override/Hidden/Conflict states persisted and recoverable after a crash. Versioned location envelope for durable destinations (C06).
- **N5.3 slice 1 done (2026-11-03, automated checks): editing engine.** Core `BookmarkEditor`: create, rename, change destination, notes and tags, move, reorder and nest; delete removes a local subtree and hides a PDF bookmark instead; "save into the PDF when possible" on local bookmarks; undo/redo of every command (500 steps; undoing a removal restores the same IDs). States follow automatically: editing a PDF bookmark makes it a LocalOverride, putting it back makes it Embedded again. Rule refined: PDF bookmarks must keep the PDF's *relative* order, so local bookmarks may sit between them. `BookmarkSearchIndex` searches titles, notes and tags with the N4.5 Arabic/Urdu matching. `OverlayTreeIndex` checks each saved change against only what it touches (Debug builds cross-check every verdict against the full rules; 6,000-round differential fuzz). Windows Release benchmark in CI with gates: 10,000-bookmark search < 100 ms p95, editor command and undo < 2 ms p95. **Next slices:** Bookmarks panel and Ctrl+B (slice 2), Command Center, Quick Access Save/Undo/Redo, Preferences → Security/Identity, Signatures panel, Auto Create Bookmarks (slice 3).
- **N5.3 slice 2 done (2026-11-03, automated checks + rendered self-review): Bookmarks panel.** The panel edits the overlay: add (Ctrl+B, ribbon Home → Bookmark, page menu, panel button) at the current view with the selected text or page as its name, ready to rename; rename (F2), delete (PDF bookmarks are hidden instead; "N hidden" bar to show them), move (Alt+↑/↓), nest/move out (Alt+→/←, mirrored in RTL), set destination to the current view, undo/redo (Ctrl+Z/Ctrl+Y), find across titles, notes and tags with breadcrumbs, expand/collapse; current-page bookmark marked by a bar and colour; state icons with words for screen readers. `BookmarkService` (one worker thread owns the profile's library index and overlay) and `BookmarkModel` (per tab; edits apply in memory at once, writes queue in order, a failed write reloads what is stored, closing a tab still writes queued edits). Identity from the library by exact path; unknown or ambiguous books are read-only. **Next (slice 3):** Command Center (Alt+Q), Quick Access Save/Undo/Redo, Preferences → Security/Identity, Signatures panel, Auto Create Bookmarks.
- **N5.3 slice 3 done (2026-11-03, automated checks + rendered self-review; Windows CI green at `9c6a172`): Command Center, Undo/Redo, Identity, Security, Signatures.** **Command Center** in the title bar (magnifier left of the window buttons, Alt+Q, as in Foxit): one field finds commands (by their name in the interface language or English, with their shortcut; commands that need a book are dimmed without one), books in the library (prefix search, off the UI thread, late answers dropped) and bookmarks in the open book (titles, notes, tags, with breadcrumbs); ↑/↓, Enter, Esc. **Quick Access Undo/Redo** and global Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z act on the open book's bookmark edits (text fields keep their own undo). **Preferences → Identity** (name, organization, email; kept on OK, exact text, 256 characters) and **→ Security** (re-check a file when returning to its tab; show the protection label in the status bar; a note that Atlas never removes protection or changes signed PDFs and keeps passwords only while a book is open); Preferences categories sorted alphabetically in the interface language, as in Foxit. **Signatures panel** (rail): signature fields with signed/empty and page, certified-document notice, and "Atlas does not check validity". The current bookmark now follows the top of the view (a chapter starting mid-page lights up when it reaches the top). Fixes: "Add This Book's Folder" button on the not-in-library notice (re-opens bookmarks after the scan); Home → Bookmark uses the plain bookmark icon (Foxit); disabled title-bar icons dim in the title-bar colour; a status-bar binding warning removed. **Decisions:** Quick Access **Save** moves to N5.4 with safe save (nothing writes PDFs before it); **Auto Create Bookmarks** becomes slice 4.
- **N5.3 slice 4 done (2026-11-03, automated checks + rendered self-review; Windows CI green at `7f4d164`): Auto Create Bookmarks. N5.3 is Implemented.** Bookmarks panel options (and the Command Center) → **Auto Create Bookmarks…**: Atlas reads every page on the text thread in short slices (other text work runs in between; progress "Checking page n of m"; Cancel stops it) and suggests headings: lines in clearly larger type than the body text, and short body-size lines that start like a heading in English, Arabic or Urdu (Chapter, Part, الباب, الفصل, المبحث, باب, حصہ …). Sizes close together are one level, largest first; running headers and footers and page numbers are left out; a heading set on two or three lines becomes one bookmark. The preview lists every suggestion with its level and page, ticked; the reader chooses up to 1–4 levels, unticks any, and places them after, before or instead of the existing bookmarks (instead = the PDF's own are hidden, Atlas's own removed). They are created as Atlas's own bookmarks in **one write and one Undo**; nothing is written into the PDF. **Limit (engine):** Qt PDF gives each line's glyph box but no font name, size or weight, so type size is estimated from the box and the letters in the line, and same-size bold headings are found only when they start like a heading; Foxit's choose-by-font-and-sample-text settings need font data from the PDF engine (an ADR-0004 decision, proposed for the owner with N5.4). Fix along the way: Theme-styled radio buttons (`AtlasRadioButton`) in Preferences, Skins and the new dialog, matching the check boxes.
- **N5.3 Complete editor:** create, rename, delete, edit destination, move, reorder, nest; duplicate names; deep Unicode trees; keyboard tree semantics; undo. **Bookmarks panel** in the rail and **Bookmark** buttons where Foxit has them (Home, Edit; Ctrl+B); Auto Create Bookmarks from the book's headings; Signatures panel (status only); Preferences → Security, Identity; **Command Center** in the title-bar search box (Alt+Q) for commands, books and bookmarks; Quick Access Toolbar gains Undo, Redo (Save arrives with N5.4 safe save).
- **N5.4 slice 1 done (2026-11-03, automated checks): outline writer, no UI yet.** [ADR-0009](docs/decisions/ADR-0009-outline-safe-save.md) (Proposed) decides what Save writes. Core `planOutlineSave`: the PDF outline after Save (visible tree; hidden PDF bookmarks kept unless removal is chosen, their children moving up; counts added/changed/kept/removed; refuses on a conflict, an unreadable destination or a PDF bookmark missing from Atlas) and destination conversion through crop box and /Rotate (0/90/180/270, round-trip tested). `QpdfOutlineWriter`: reads the outline, pages, boxes and rotation with `qpdf --json=2`; builds an `--update-from-json` document with a fresh outline item set (existing items keep every key but the tree links; /Count keeps open/closed); writes to a separate file and runs `qpdf --check`; verifies the result by qpdf's own reading (outline tree, titles, destinations; unchanged items identical; pages, boxes, rotation, resources, annotations, document info, encryption, forms and names unchanged). Tested on the N2 fixtures: edits in English and Arabic with a new folder and a move (Qt PDF reads the result), a rotated and a cropped page, an RC4-encrypted PDF (encryption, user and owner passwords kept), and refusals. **Next (slice 2):** the safe-save pipeline (preflight, temporary file, Qt PDF reopen, backup, atomic replace, overlay commit to Embedded) with failure injection, then Save in the UI.
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
- **Comment ribbon tab** with Foxit's groups and keys for the tools above (Shift+H highlight, Ctrl+U underline, Shift+S strikeout, Ctrl+Shift+N note, Shift+T typewriter, Shift+P pencil); Comments panel in the rail (sort by page, type, author, date, colour; filter; search); Keep Tool Selected; Show/Hide all comments (Ctrl+8 / Ctrl+Shift+8); open/close all pop-ups; Search & Highlight; Export Highlighted Text (Ctrl+I); comment context menu (reply, properties, set as default); Preferences → Commenting, Tablet.
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

- Printing with preview, ranges, labels, layout and grayscale (File → Print, Ctrl+P); source hash unchanged.
- Moved from N4.4/N4.5: Open a Copy; image context menu (copy, save image as); Destinations panel; open PDF links in a new tab; thumbnail size; single-key accelerators (off by default); Preferences → Full Screen, Reading (reading order, caret browsing) and rendering options.
- Foxit File page and Preferences completed: Properties (Ctrl+D), Preferences categories for every shipped feature, Quick Access Toolbar customization (above/below ribbon), Help tab (offline User Guide F1, Command Line Help, Set as default PDF reader, Report Error as a local diagnostic bundle); optional classic menu bar (File, Edit, View, Tools, Help); minimize to tray; print current/all tabs; Preferences → Print, File Associations.
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

## N9 — Languages + accessibility qualification (`2.0.0-beta.7`)

**Goal:** close the remaining gaps across everything built. This is qualification, not the first time these are done.

Accessibility ribbon tab completed: Reading Option (reading order, page/document reading), Setup Assistant (first-run accessibility settings), Reading Order view. Run the full workflow matrix (Library, Reader, Bookmarks, protected/local, conflicts, import/export, ink, print, metadata, backup/migration) in the English, Arabic and Urdu interfaces with an any-script content corpus (Arabic, Urdu, Persian, English, Hebrew, Devanagari, CJK, mixed direction), keyboard-only, with Narrator, Accessibility Insights, 100%/200%, high contrast and reduced motion (C13).

### Automated QA

- Translation completeness in all three languages with no English fallback in core screens; Urdu Nastaliq and Arabic Naskh rendering checks.
- Bidi isolation, exact Unicode round trips, normalization, search and collation.
- Keyboard traversal and UI Automation role/name/state assertions; scale and high-contrast checks; a coverage report with no blank core cell.

### Owner review

Repeat the matrix above personally in English, Arabic and Urdu, focusing on reading comfort, translation quality and Narrator flow.

### Stop gate

**`N9 PASS`**, with no core workflow untested or failing in the published matrix.

---

## N10 — Speed, hardware + release candidate (`2.0.0-rc.1`)

**Goal:** prove the feature-complete build on real hardware and package it.

Large libraries; long PDFs; 10k+ bookmark trees; SSD and HDD/external drives; pen and touch; multiple DPIs and monitors; a physical printer; interrupted saves, conflicts and offline folders; long sessions; install/upgrade candidate; SBOM, notices and security review; network-observed offline run (C05, C14, C22).

Moved from N4: tiled rendering for pages beyond 8,192 px (clip renders); untagged-document prompt and tagged reading order for screen readers; one shared in-memory copy of each PDF for pages, thumbnails and text (today up to three).

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

Installer technology chosen and qualified here; installer and executable signed with DerekSol's code-signing certificate so Windows names the publisher ([WORKFLOW.md](docs/WORKFLOW.md#5-versions-and-releases)); clean install/upgrade/uninstall; optional, user-controlled `.pdf` association; uninstall never deletes books, research or backups; release notes with known limitations; source, tag, artifact, checksum, SBOM and notices archived.

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
| Qt PDF returns duplicate raw link rows | N4.5 (implemented; Qt 6.6+ link model) | De-duplication test plus link-heavy fixture (Windows CI) |
| Rotated explicit destinations need rotation metadata | N4.5 | Exact targets on normal, cropped, 90°, 180° and 270° pages |
| Qt PDF search slower than PDFium on synthetic tests | N4.5 | Long-book p50/p95/p99, cancellation, no missed results |
| Qt PDF memory trend needs real-reader proof | N4.2, N10 | 2,000-page and multi-tab memory series |
| Fit/FitH/FitV handling | N4.4 | Destination-mode fixtures |
| Broader graphics/annotation rendering corpus | N4, N6 | Image, gradient, transparency and annotation subtypes |
| Security-handler breadth | N5.1 | Modern and unsupported encryption fixtures, no bypass |
| Signature presence ≠ validity | N5.1 (detect only); validation after 2.0 | Signed originals preserved; never a "valid" claim |
| One corrected Arabic/Urdu page is not a corpus | N4, N9 | Several real-font Arabic/Urdu PDFs: joining, diacritics, search, direction |

## Competitive action IDs (C01–C22)

Kept for traceability with the [2026-09-27 audit](docs/history/plan-v1/docs/COMPETITIVE_GAP_ACTIONS.md). In 2.0: C01 (N3, done) · C02–C05, C15, C22 (N4) · C06 (N5) · C07, C08, C16, C19 (N6) · C09, C10, C21 (N7) · C12 (N8) · C13 (N9) · C14 (N10–N11). **After 2.0:** C11 read-aloud, C17 side-by-side, C18 reflow view, C20 form filling; see [AFTER_2_0.md](docs/AFTER_2_0.md).
