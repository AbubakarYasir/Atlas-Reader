# Atlas Reader — Plan to 2.0

<!-- atlas-status: N4|in-progress -->

This is the single source of direction: what Atlas is, what Windows 2.0 must contain, what comes later, and how we judge quality. Order and status live in [CHECKPOINTS.md](CHECKPOINTS.md). The detailed contracts live in the [docs folder](docs/README.md).

**Owner:** Abu Bakar (`AbubakarYasir`) · **Current version:** `2.0.0-beta.1` · **Active stage:** N4 Reader · **Target:** Windows 11, then other platforms after 2.0 · **Interface languages:** English (default), Arabic, Urdu · **Content:** any language

## 1. North star

> **Find the book → read it smoothly → navigate and research it deeply → keep that research safe.**

Atlas is a local-first reading and research environment for serious readers: students of knowledge, researchers and anyone who works across many long books, often in Arabic, Urdu and English together. It looks and works like the desktop PDF tool its users already know (Foxit's workspace, [ADR-0007](docs/decisions/ADR-0007-foxit-workspace-parity.md)) and competes by being **exceptionally good at a few workflows**: Foxit's other features arrive in stages, before or after 2.0.

What makes Atlas different is the combination:

1. **Library and reader as one product.** It is not a "File → Open" viewer.
2. **Deep bookmarks as the main research object.** They are not a narrow sidebar feature.
3. **Portable when possible, local when necessary.** Protected, read-only, signed or offline PDFs stay fully usable for research.
4. **A complete English, Arabic and Urdu interface** in the first version (English is the default), with right-to-left layouts that are designed rather than translated, and **any language accepted** in everything you write or read: bookmarks, notes, tags, annotations and search. Foxit and Acrobat offer no Arabic or Urdu interface; Atlas does.
5. **Accessibility** built into custom reader and tree controls.
6. **Speed you can measure**, even with large books and large libraries.
7. **Honest states:** no silent overwrite and no false "saved".
8. **A mature desktop workspace** (Foxit's layout with Atlas's colours) that feels made by an experienced product team.

## 2. Principles

| Principle | Meaning |
|---|---|
| **Index → Reader → Bookmarks** | Core work happens in this order; later features never weaken it |
| **Never lost silently** | No false save success, blind overwrite, guessed relinking or purge of offline roots |
| **Portable when possible, local when necessary** | Research goes into the PDF only when safe and permitted; otherwise it is kept locally, clearly labeled and exportable |
| **The interface never waits** | Parsing, rendering, scanning, hashing, saving and large queries never run on the UI thread |
| **Measure, don't guess** | Speed claims need numbers on named hardware; profilers find bottlenecks, not intuition |
| **Three languages and accessibility from day one** | Every user-facing stage ships complete English, Arabic and Urdu strings, RTL layouts, keyboard, Narrator, 200% scale and high contrast. No screen ships with a missing translation |
| **Any language in your content** | Titles, bookmarks, notes, tags, annotations and search accept and preserve every Unicode script exactly (Arabic, Urdu, Persian, English, Hebrew, Devanagari, CJK and more), including mixed-direction text |
| **Familiar, not invented** | Foxit's proven workspace, sizes and shortcuts ([FOXIT_PARITY.md](docs/FOXIT_PARITY.md)); no generic "AI app" look, no gradients or glow, no clutter |
| **Replaceable engines** | PDF libraries sit behind Atlas-owned interfaces; vendor types never reach the core |
| **Portable core** | Shared rules contain no Windows-only types, so Android/macOS/iOS stay possible |
| **Evidence before claims** | Planned → Implemented → Verified → Accepted → Released are distinct states |
| **Offline by default, private always** | No accounts, telemetry, uploads or update checks in 2.0. After 2.0, connected features (cloud places, e-signature services, assistants, updates) are opt-in, explain what leaves the computer, and never need an Atlas account |
| **Focus beats feature count** | A feature that threatens the four core steps carries a higher burden of proof |

## 3. When goals conflict

Use this order:

1. Research data safety and integrity
2. Correctness and interoperability
3. Responsiveness of core workflows
4. Accessibility, Arabic/RTL and clarity
5. Visual quality and beauty
6. Resource efficiency
7. Feature breadth

Beauty ranks high and is a release requirement (see [DESIGN.md](docs/DESIGN.md)), but it can never hide a failure above it.

## 4. Windows 2.0 scope

Priority labels: **P0** cannot ship without it · **P1** required for 2.0, after the core is dependable · **P2** nice to have, may move after 2.0 with an owner decision. The checkpoint that delivers each item is shown in brackets.

### 4.1 Library and index — P0 (accepted in N3, extended in N5/N7)

- Multiple library folders, including removable and offline drives; offline folders are kept, never purged.
- Open single files outside library folders without silently adding their parent folder.
- Background, progressive, cancellable scanning; safe overlapping folders; protection against junction/symlink loops.
- Guarded document identity: moves and renames keep research; copies stay separate; ambiguous matches are never guessed.
- Clear per-book states: available, locked, read-only/restricted, signed, offline, missing, cloud placeholder, unreadable, changed, unsupported.
- Filename-first names (extension kept separately and protected); embedded title and author shown as secondary.
- Search across metadata, with a documented Arabic/Unicode normalization policy; Favorites, Recents and folders.
- Global **Command Center** in the title-bar search box (`Alt+Q`, Foxit's key) for commands, books and bookmarks [N5].
- The Library is the pinned first document tab (Foxit's *Start* tab) [N4.3].
- Sorting, filters, safe rename, collections and tags [N7].

### 4.2 Reader — P0 [N4]

- Atlas-owned virtualized viewport: only visible and nearby pages are rendered; memory stays bounded on 2,000+ page books.
- Continuous and single-page modes, facing and cover-page spreads, reading direction independent of UI language.
- Free zoom, actual size, fit page/width; exact page jump; page labels separate from page numbers; optional academic page offset.
- Correct rotation, crop/media boxes and mixed page sizes.
- Tabs with bounded resources; opt-in session restore (off by default; never stores passwords).
- Navigation panel: Outline · Pages (thumbnails) · Bookmarks · Annotations (from N6).
- Text selection and copy, cancellable document search, internal and external links, Back/Forward history.
- Focus/full-screen reading with a discoverable exit; per-book position and zoom remembered.
- Foxit's ribbon tools for reading: Hand, Select text, SnapShot, Zoom (marquee, zoom to), Fit Visible, AutoScroll, Reverse View (right-to-left page order), page display buttons, Word Count [N4.4–N4.5].
- Image-only PDFs read normally, with an honest "no text layer" state. There is no OCR.
- Offline safety: no document JavaScript, launches or remote fetches; external links need explicit confirmation.

### 4.3 Bookmarks and outlines — P0, the signature feature [N5]

- Unlimited practical depth, duplicate names under different parents, Arabic/Urdu/mixed-direction titles, full breadcrumbs, exact destinations, reveal current section.
- Complete editing: create, rename, delete, edit destination, move, reorder, nest/unnest, undo; full keyboard and screen-reader operation.
- Storage states shown honestly: **Embedded · Local only · Pending embed · Local override · Locally hidden · Conflict**.
- Restricted, read-only, signed, locked and offline PDFs remain bookmarkable locally; work can be embedded later, or saved as a copy.
- External changes are detected and reconciled, never overwritten.
- Safe save: temporary write → validate → backup → replace → reopen → only then "Embedded".
- Lossless versioned `.atlas-bookmarks.json`, plus Markdown and CSV export (all, subtree or local-only); import with preview and append/merge/replace/local-only modes.
- 10,000-bookmark search and navigation under 100 ms p95.

### 4.4 Ink and annotations — P1 [N6]

- Low-latency pen with pressure where the hardware supports it reliably; freehand highlighter; text highlight, underline and strikeout on real text.
- Color, opacity and width presets; stroke and partial erasing; lasso/rectangle selection; move, resize, copy, duplicate and delete; undo/redo.
- Typed notes, sticky notes, and line/arrow/rectangle/ellipse markup.
- Annotation list with search, filter, jump and edit; show/hide without deleting.
- Standard PDF structures that reopen correctly in an independent reader; local fallback for protected files.
- Annotation export as versioned JSON (lossless, re-importable) and Markdown (readable report).
- Physical pen tests: stylus-button eraser, palm/touch separation, and a recovery budget after a crash.

### 4.5 Everyday tools — P1 [N7]

- Printing: printer choice, preview, page and label ranges, copies, orientation, paper size, scaling, grayscale. The source file stays byte-identical.
- Covers: generated from page one, with correct cache invalidation and a fallback for locked or offline books.
- Book details panel that separates PDF values from Atlas-local values; selected metadata editing with preview through the N5 safe-save path.
- Sort (filename, title, author, last opened), filters with one-step clear, safe filename rename separate from metadata editing.
- Local collections and tags that never move or delete files.
- Windows integration: Open With, drag and drop, clipboard, long and Unicode paths, and no lost work when Windows shuts down or the session ends.
- Preferences window like Foxit's (searchable category list; Ctrl+K): General · Languages · Skins · Page Display · Reading · Accessibility · History · Documents · Commenting · Print · Security · Library (Atlas) · Backup (Atlas). Quick Access Toolbar customization; Set as default PDF reader; offline User Guide (F1) and Command Line Help.
- Privacy-safe rotating logs; a diagnostic bundle the user generates and previews.

### 4.6 Backup and migration — P0 before stable [N8]

- Versioned Atlas backup of local research (overlays, tombstones, notes, tags, collections, favorites, progress, folders, identity links, settings). Books are excluded by default.
- Restore with preview, validation and explicit unresolved books; never relink by filename alone.
- Read-only, repeatable import from the old Flutter app's profile; the old data is never modified.

### 4.7 Cross-cutting — P0 [every stage, qualified in N9–N10]

- **Interface:** English (default), Arabic and Urdu, switchable live and remembered; every string translated in all three; mirrored right-to-left layout for Arabic and Urdu; Urdu shown in Nastaliq (Noto Nastaliq Urdu), Arabic content in Naskh; bidi isolation for paths, file names and mixed text; localized digits and dates where the language expects them, without ever changing stored data.
- **Content in any language:** every text field, bookmark, note, tag, annotation and search box accepts any Unicode script through the Windows keyboard/IME, stores it byte-exact, renders it with system font fallback, and searches it with a documented per-script normalization (Arabic/Urdu letter variants, diacritics, tatweel; case folding for Latin).
- Keyboard-only operation for every core task; Narrator/UI Automation names, roles and states; visible focus; 100% and 200% scale; high contrast; reduced motion; never color alone.
- Performance budgets from [QUALITY.md](docs/QUALITY.md) met on named hardware.
- Installer, upgrade and uninstall that never delete books or research; optional file association [N11].

## 5. After 2.0 and never

**Owner direction (2026-11-01): Atlas will cover every feature of Foxit PDF Editor at some point.** What a reader and research tool needs is in 2.0 (sections 4.1–4.7); everything else is planned in eleven after-2.0 tracks, each a `2.x` release line with its own review ([AFTER_2_0.md](docs/AFTER_2_0.md)); the command-by-command map is [FOXIT_PARITY.md §3](docs/FOXIT_PARITY.md#3-complete-foxit-feature-catalogue-and-atlas-plan):

- **A Reading Plus:** tab groups and split view, reflow and Text Viewer, read aloud, rulers/guides, loupe/magnifier, compare, page transitions, multiple windows
- **B More formats:** EPUB · CBZ/CBR · DRM-free AZW3/MOBI/PRC · KFX feasibility
- **C Pages, Create, Convert:** all Organize commands, watermark/header/footer/Bates, create and combine, export to images, text, Office and HTML
- **D Edit and rich annotations:** text/object/image editing, links, cross references, articles, stamps, measure, attachments, layers, 3D and multimedia
- **E Forms:** filling, authoring, form data, sandboxed JavaScript
- **F Protect and Sign:** redaction, sanitize, encryption, Fill & Sign, digital signatures and validation
- **G OCR, accessibility authoring, standards:** offline OCR (English/Arabic/Urdu), autotag and checker, PDF/A, preflight, optimizer
- **H Automate and customize:** Action Wizard, batch, ribbon/shortcut customization, portfolio, binder, Windows/Office integration, signed plug-ins
- **I Review and share:** e-mail, shared-folder and e-mail reviews, Tracker, comment status
- **J Connected (opt-in):** cloud places, document-management systems, e-signature services, rights management, on-device-first assistants, updates
- **K Platforms:** Android → Linux → macOS → iOS/iPadOS

Until those ship, 2.0 shows honest "not supported yet" states, for example for forms, XFA, attachments and signatures (detected but not validated).

**Never:** password cracking, removing restrictions without the owner password, DRM bypass, hidden telemetry or uploads without consent, unsigned or unsandboxed executable plug-ins.

## 6. What "excellent" means for 2.0

"Perfect" is not a measurable goal, so Atlas uses these. 2.0 ships only when **all** are true:

1. Every P0/P1 item above is Accepted by the owner.
2. No known path loses research or silently changes a user's PDF.
3. Protected, signed, read-only, offline and conflicted PDFs have tested behavior.
4. Speed budgets pass on named hardware: first page, scrolling, search, 10k bookmarks, ink latency, memory on 2,000-page books.
5. Embedded bookmarks and annotations round-trip in an independent PDF reader.
6. The English/Arabic/Urdu × keyboard × Narrator × 200% × high-contrast matrix has no untested core workflow, and the any-script content corpus round-trips exactly.
7. The design review in [DESIGN.md](docs/DESIGN.md) passes on every screen, in every state, in both languages and themes.
8. Backup/restore and legacy migration are proven on clean profiles.
9. Install, upgrade and uninstall preserve user data; licenses, notices and SBOM are complete.
10. Side-by-side comparisons on the same files show Atlas at least matching Foxit/Acrobat/SumatraPDF on its core tasks, and leading on bookmarks, safety and Arabic.

## 7. Competitive references

**Owner decision (2026-11-01, second): Atlas reproduces Foxit PDF Editor's workspace** ([ADR-0007](docs/decisions/ADR-0007-foxit-workspace-parity.md)): title bar with Quick Access Toolbar, ribbon tabs and grouped commands, fixed-width document tabs with the Library as the first tab, left navigation rail (F4), status bar, File page, Preferences window, skins and Touch mode, at Foxit's measured sizes and with Foxit's shortcuts. Atlas keeps its own icons (Fluent, MIT), colours and name, and never extracts anything from Foxit's program files.

Every Foxit command is mapped to an Atlas stage in **[docs/FOXIT_PARITY.md](docs/FOXIT_PARITY.md)**: what lands now, what lands before 2.0 (reader, bookmarks, annotations, everyday tools, accessibility), and **everything else in after-2.0 tracks A–K** (organize, create/convert, editing, forms, OCR and accessibility authoring, protect and sign, automation, review and share, opt-in connected features). Only bypasses, cracking and hidden data collection are never built.

Where Atlas goes further: **Arabic and Urdu interface languages** (Foxit ships neither), the Library as a research home, deep local-fallback bookmarks, and honest storage states.

Other products we learn from:

| Product | What it teaches | Atlas response |
|---|---|---|
| Acrobat | Security, signatures, annotation interoperability | Match safety expectations; never bypass |
| Foxit | Responsive desktop reading, complete bookmark editing | Equal editing depth plus local fallback |
| SumatraPDF | Instant startup, minimal friction | Startup and simple reading are measured from N1 |
| Xournal++ | Pen-first feel, separate local layer | Live ink never waits on PDF saving |
| Librera / Okular | Library flow, open multi-platform reading | A research library, not a file picker |

At the end of each stage we run the same tasks on the same files in a relevant mature product and record version, task, result and what Atlas must improve. Comparisons never become marketing claims without measurements.

## 8. Main risks

| Risk | Level | Guard |
|---|---|---|
| Silent PDF or research corruption | Critical | Safe-save pipeline, preservation tests, failure injection, local fallback |
| Overwriting a file changed by another app | Critical | Revision check before every write; reconcile instead |
| Research attached to the wrong copy | Critical | Guarded identity ([ADR-0005](docs/decisions/ADR-0005-document-identity-and-reconciliation.md)); no filename-only matching |
| Licensing mistake (Qt LGPL, fonts, codecs) | Critical | Admission checklist, notices/SBOM every release |
| Scope creep delaying 2.0 | High | **Focused 2.0** (section 5); new ideas go to [AFTER_2_0.md](docs/AFTER_2_0.md) |
| UI stalls or memory blow-up on long books | High | Thread rules, virtualized viewport, budgets, 2,000-page memory series |
| Arabic/RTL or accessibility retrofitted late | High | Required at every stage, not only N9 |
| Windows assumptions leaking into the core | High | Ports/adapters; storage references, not paths |
| Owner review becoming a bottleneck | Medium | One batched review per stage with a short plain-language guide |
| Docs drifting from code | Medium | One status marker, automated link/status check in CI |

## 9. Decision log

| Date | Decision |
|---|---|
| 2026-09-22 | Native C++23 + Qt Quick replaces Flutter ([ADR-0001](docs/decisions/ADR-0001-native-stack.md)); N0 and N1 accepted |
| 2026-09-27 | Qt PDF 6.10.3 for reading/rendering and qpdf 12.4.1 CLI for structure/security/writes ([ADR-0004](docs/decisions/ADR-0004-pdf-engine-responsibilities.md)); N2 and N3 accepted |
| 2026-09-27 | Scope was expanded to multi-format and offline editing before 2.0 (now superseded) |
| 2026-09-28 | `2.0.0-beta.1` released; N4 started |
| **2026-11-01** | **All Foxit features, Atlas logo colours.** Owner: cover every Foxit PDF Editor feature at some point, using the owner's Foxit UI reference package (menus, dialogs, modules, plug-ins, skins, runtime screenshots). FOXIT_PARITY §3 now catalogues all of them; AFTER_2_0 is reorganized into tracks A–K; items previously "never planned" (Office conversion, OCR, measuring, 3D/multimedia, JavaScript, review/share, cloud, assistants) are planned, connected ones opt-in. **Atlas's main colour is built from its logo** (Atlas Blue, navy, cyan) instead of green; Foxit's neutral greys stay |
| **2026-11-01** | **Foxit workspace parity** ([ADR-0007](docs/decisions/ADR-0007-foxit-workspace-parity.md)). Owner: copy Foxit's UI/UX as exactly as possible so Atlas never looks like a generic AI app. Layout, measurements, command placement and shortcuts follow Foxit; icons, colours and brand stay Atlas's. Foxit's features are staged in [FOXIT_PARITY.md](docs/FOXIT_PARITY.md): necessary ones before 2.0, others after. Offline OCR, image/text export and stamps move from "never" to after-2.0 candidates. Supersedes the "concepts only, slim command bar" decision of the same morning |
| **2026-11-01** | **Trilingual interface and any-language content.** English (default), Arabic and Urdu UI ship complete in the first version; all user content accepts any language. **Foxit concepts adopted** (section 7) for familiarity and accessibility, with Atlas's own visual design. **UI revamp moved forward** to N4.3 so every later reader feature is built into the new workspace |
| **2026-11-01** | **Focused 2.0.** EPUB/comics/Kindle, PDF editing tools (E1–E5), form filling, side-by-side, reflow view and read-aloud move after 2.0. **Batched owner reviews:** one review per stage instead of per sub-stage. **Docs consolidated:** 35 reference files in `docs/` became 9 plus an index, and `INFO.md` was merged into README/CHECKPOINTS; the previous set is kept unchanged in [docs/history](docs/history/README.md) |

Every change to scope, priority or release gates is added here with its reason.

## 10. Where things live

| Need | Document |
|---|---|
| Status, stage tasks, tests, review gates | [CHECKPOINTS.md](CHECKPOINTS.md) |
| Look, feel, accessibility, Arabic/RTL | [docs/DESIGN.md](docs/DESIGN.md) |
| Foxit layout measurements and feature map | [docs/FOXIT_PARITY.md](docs/FOXIT_PARITY.md) |
| Layers, threads, engines, stack | [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) |
| Document safety, data formats, security | [docs/DATA_AND_SAFETY.md](docs/DATA_AND_SAFETY.md) |
| Tests, evidence, speed budgets | [docs/QUALITY.md](docs/QUALITY.md) |
| Branches, commits, releases | [docs/WORKFLOW.md](docs/WORKFLOW.md) |
| Building | [docs/BUILDING.md](docs/BUILDING.md) |
| Libraries, tools, licenses | [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) |
| After 2.0 | [docs/AFTER_2_0.md](docs/AFTER_2_0.md) |
| Rules for coding agents | [AGENTS.md](AGENTS.md) |
