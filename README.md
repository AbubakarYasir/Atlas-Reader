# Atlas Reader

> Fast, offline-first book discovery, deep PDF bookmarks, and pen writing for
> Windows—without the crowded interface or a cloud account.

| Release | Status | Formats | Primary platform |
|---|---|---|---|
| `0.8.0-beta.2` (build 4) | Corrected C1 prerelease candidate ready for owner test; latest accepted release is `0.8.0-beta.1` | PDF reader/editor; PDF and EPUB discovery | Windows 11 |

Atlas Reader focuses on three jobs:

1. Find books quickly across one or more folders.
2. Create detailed bookmarks and jump directly to the right book and page.
3. Read, highlight, and write on PDFs with responsive pen tools.

The guiding rule is **the document is the database**. PDF outline bookmarks and
standard `/Ink` annotations are written back into the PDF, while a local SQLite
index makes browsing and search fast. Atlas works offline and does not require
an account.

> Atlas is beta software that modifies local PDFs. Saves are validated and use
> recoverable temporary replacement, but irreplaceable books should still have
> an independent backup.

## Highlights

- Open any PDF directly—no library folder required.
- Scan multiple folders recursively for PDF and EPUB books.
- See books progressively while a long scan is still running.
- Search by title, author, file name, bookmark, and nested breadcrumb.
- Use Cover Grid, Detailed List, or Compact List.
- Sort by title, author, date added, last opened, or reading progress.
- Use Library, Recents, Bookmarks, Favorites, Folders, and Settings from one
  compact desktop shell.
- Read PDFs with continuous scrolling, zoom, page jump, margin crop,
  brightness, and Day, Night, OLED, or Warm Parchment display themes.
- Browse and search unlimited nested PDF outlines and local bookmarks.
- Add a bookmark to the active page with a title, description, and tags.
- Press `Ctrl+K` to find a book or bookmark and jump to the exact page.
- Draw pressure-sensitive pen strokes and translucent highlights; choose five
  standard colors and line sizes; erase; undo; and redo.
- Save interoperable PDF `/Ink` annotations with correct 72-point coordinates,
  page isolation, and PDF bottom-left coordinate conversion.
- Use an English or Arabic interface with RTL layout and mixed-script titles.
- Operate core bookmark and save workflows from a keyboard or screen reader.

## Install on Windows 11

Atlas does not yet publish a signed installer. The current beta is built from
source.

### Prerequisites

- [Git for Windows](https://git-scm.com/download/win)
- [Flutter SDK](https://docs.flutter.dev/get-started/install/windows/desktop)
- Visual Studio 2022 with **Desktop development with C++**

Confirm the Windows toolchain in PowerShell:

```powershell
flutter doctor
```

Resolve every Windows desktop item reported by Flutter before continuing.

### Clone and run

```powershell
git clone git@github.com:AbubakarYasir/Atlas-Reader.git
cd Atlas-Reader\atlas_poc
flutter pub get
flutter run -d windows
```

If GitHub SSH is not configured, download the repository ZIP, extract it, open
PowerShell inside `atlas_poc`, and run the final two commands.

### Build the release executable

```powershell
flutter build windows --release
.\build\windows\x64\runner\Release\atlas_poc.exe
```

The executable and its adjacent runtime files must remain together. A packaged
installer and optional PDF file association are pre-1.0 work.

Versioned beta ZIPs, checksums, verification notes, and limitations are
published on the project's
[GitHub Releases](https://github.com/AbubakarYasir/Atlas-Reader/releases) page.

## Quick start

### Open one PDF immediately

Choose **Open PDF** from the top of the main window. The file does not need to
be under a library folder. Atlas remembers it in **Recents** without silently
registering or scanning its parent folder.

You can also pass a PDF path when launching Atlas:

```powershell
.\build\windows\x64\runner\Release\atlas_poc.exe "C:\Books\My Book.pdf"
```

This command can be selected from Windows Explorer's **Open with** dialog.
Atlas does not change the system's default PDF application automatically.

### Build a library

1. Open **Folders**.
2. Choose **Add folder** and select a location containing PDF or EPUB books.
3. Add more locations if needed.
4. Watch the indexed count update while books appear in **Library**.
5. Use **Scan now** whenever you want an immediate full refresh.

Atlas watches registered locations for additions, changes, renames, moves, and
removals while it is running. Removing a folder from Atlas deletes only its
local index records; it never deletes the folder or its books.

### Find and organize books

- **Library** shows all indexed and directly opened books.
- **Recents** shows opened books, newest first.
- **Favorites** shows starred books.
- **Folders** provides an explorer-style view of registered roots and files.
- Type in the Library search box to match title, author, or file name instantly.
- Choose Cover Grid, Detailed List, or Compact List.
- Sort by Title, Author, Date Added, Last Opened, or Reading Progress; reverse
  the order with the adjacent arrow.

EPUB files can be discovered, indexed, searched, sorted, and favorited. The
full reader, bookmarks embedded into the document, and pen writing currently
require PDF.

## Reader and bookmarks

### Read and navigate

Open a PDF from any main destination, `Ctrl+K`, **Open PDF**, Explorer, or a
launch command. The normal reader is byte-backed and virtualized so Windows can
release the source file before a save.

- Scroll continuously with a wheel, trackpad, touch, or keyboard.
- Zoom with the reader controls.
- Jump by page number or the chapter-aware scrubber.
- Open the outline drawer to expand and search chapters, sections, deeper
  descendants, and local bookmarks.
- Change display theme, reading mode, margin crop, brightness, and academic
  page offset from the reader settings button.

### Add a bookmark

1. Go to the target page.
2. Select the bookmark button or press `Ctrl+B`.
3. Enter a title and, if useful, a description and comma-separated tags.
4. Save.

Atlas safely commits the authoritative PDF outline first, then updates SQLite.
This order prevents a Windows file-lock or replacement failure from leaving a
misleading local-only bookmark. The active page number is filled automatically.

### Find a bookmark and jump to it

- Open **Bookmarks** to browse or filter all local bookmarks.
- Select a result to open its PDF at the exact saved page.
- Press `Ctrl+K` from anywhere for quick open. Results include the book title,
  full nested breadcrumb, and page number.
- Use **Organize & sync** for tree editing, tags, importing external outline
  changes, diff preview, and committing local tree changes.

The outline supports sub-bookmarks at arbitrary depth. Identical section names
under different parents remain distinct because sync identity uses the full
hierarchical path, not only the visible title.

## Pen, highlighter, and PDF save

1. Open a PDF and choose **Write**.
2. Choose **Pen** or **Highlighter**, a color, and a thickness.
3. Draw on the active page. Drawing gestures are separated from navigation, so
   a pen drag does not scroll or flip the page.
4. Use **Area eraser**, or choose **Select** and delete a complete stroke.
5. Use `Ctrl+Z` and `Ctrl+Shift+Z` for undo and redo.
6. Choose **Save** or press `Ctrl+S`.

Each stroke is bound to one zero-based PDF page index. The editor converts
screen positions to native PDF points (72 points per inch), accounts for the
top-left versus bottom-left origin difference, and stores standard `/Ink`
annotations. A `RepaintBoundary` isolates live drawing from the underlying PDF
render, and Select mode restores normal scrolling.

Before Atlas replaces the source PDF, it verifies page count, nested outline
paths and destinations, document information, and XMP presence. It then:

1. Writes `<book>.pdf.tmp`.
2. Opens and validates the temporary PDF.
3. Moves the original to `<book>.pdf.atlas-backup`.
4. Promotes the validated file.
5. Restores the original when replacement fails, where possible.

The normal reader and writing editor use completed byte reads rather than an
open source-file handle, avoiding common Windows sharing errors (`errno 32`).

## Keyboard and accessibility

| Action | Shortcut |
|---|---|
| Quick open and global bookmark search | `Ctrl+K` |
| Bookmark the active reader page | `Ctrl+B` |
| Save/commit current PDF changes | `Ctrl+S` |
| Undo ink action | `Ctrl+Z` |
| Redo ink action | `Ctrl+Shift+Z` |
| Rename a focused bookmark | `F2` |
| Remove a focused bookmark | `Delete` |
| Open/select focused tree item | `Enter` |
| Move or expand/collapse outline focus | Arrow keys |

Interactive controls provide semantic labels. Save, sync, and error outcomes
are announced without moving keyboard focus. Arabic mode mirrors Material
layout, and Arabic, English, numbers, punctuation, and mixed-script bookmark
titles retain their Unicode content.

## Settings and user control

Settings is organized into clear categories:

- **Library & Storage:** open an external PDF or manage indexed folders.
- **Appearance & Language:** System, Light, or Dark app theme; English or
  Arabic interface.
- **Reader & Writing:** discover all per-document display and ink controls.
- **Accessibility & Keyboard:** shortcuts and assistive-technology behavior.
- **Data & Safety:** document-authority and local-index guidance.

Per-document reading controls stay inside the reader, where their effect is
visible immediately. Folder addition, removal, and rescanning remain explicit;
Atlas never enrolls an entire directory just because one PDF was opened.

## Back up and recover data

Close Atlas before copying its database.

1. Back up every edited PDF. Portable outlines and `/Ink` live in those files.
2. Back up `atlas_db.sqlite` from the Windows application Documents directory.
   The typical location is under your user Documents folder; search for the
   exact filename if Windows has redirected Documents through OneDrive.
3. Keep any `.pdf.atlas-backup` file until the corresponding PDF has been
   opened and checked.

To restore, return the PDFs to accessible paths and copy `atlas_db.sqlite` back
while Atlas is closed. A rebuilt local index can rediscover PDF outlines and
ink, but local-only descriptions, tags, favorites, and reading progress require
the SQLite backup.

## Privacy and data ownership

- Offline by default; no Atlas account is required.
- No cloud upload or hosted synchronization in this beta.
- Folder scanning reads metadata and outlines but does not modify a book.
- A PDF changes only after an explicit bookmark, annotation, or sync save.
- Source document operations pass through a platform-neutral file-system
  interface; Windows is the current adapter.

## Current beta limitations

- PDF is the only full reading and writing format. EPUB is discovery-only.
- Bookmark titles and hierarchy are portable through the standard PDF outline.
  Descriptions and tags remain in SQLite; a portable interoperable metadata
  format is a known pre-1.0 gap.
- Directly opened PDFs are remembered in Library/Recents, but their parent
  folder is not watched unless the user adds it explicitly.
- PDF cover thumbnails are not yet rendered for every scanned PDF; EPUB cover
  extraction and a clean generated fallback are available.
- There is no signed installer or automatic Windows file association yet.
- The current automated and native-runtime target is Windows. Android storage
  and input adapters are planned after the Windows core is stable.
- Historical research, split-workspace, speed-reading, and hands-free modules
  remain in the repository but are intentionally absent from the focused main
  reader. TTS, cloud services, and other secondary stages are not in scope.

## Verification status

Verified on Windows on 2026-09-10:

```text
flutter analyze                                            0 issues
flutter test                                               56 passed, 2 opt-in audits skipped
flutter test integration_test/windows_reader_workflow_test.dart -d windows
                                                           3 passed
flutter build windows --release                            succeeded
```

The native integration runner covers these scenarios:

- A launch-argument PDF outside all library folders opens and enters Recents.
- A mouse stroke saves as standard `/Ink`, stays on its source page, and is
  visible after reopening.
- Existing nested Arabic/English outline structure and document metadata remain
  intact after ink save.
- Two folders scan and index Arabic and English metadata.
- Library filtering and Command Center bookmark selection return the expected
  file and exact page.

The two skipped unit tests are read-only smoke tests that require personal
Arabic and English PDF paths supplied through environment variables.

## Roadmap and owner checkpoints

Development follows the [checkpoint roadmap](CHECKPOINTS.md) to stable `1.0`.
Only one checkpoint is active at a time. After its automated gates pass, work
stops for the numbered owner test; the next checkpoint does not start until the
owner explicitly records a PASS.

The owner accepted **C0 — Baseline acceptance** on 2026-09-09. **C1 — Reader
workspace** build 4 is verified and stopped at **Ready for owner test**; C2 will not
start until the owner records an explicit PASS.

## Developer guide

### Technology

- Flutter and Dart
- Drift + SQLite/FTS5 for local indexing
- `dart_pdf_editor` for a shared Read/Write viewer and standard PDF editing
- `pdf_document` for outline operations and PDF inspection
- Platform-neutral `DocumentFileSystem` with a Windows implementation

### Project structure

```text
lib/
├── core/
│   ├── database/       SQLite connection infrastructure
│   ├── epub/           EPUB metadata and cover discovery
│   ├── covers/         Local cover cache
│   └── file_system/    Platform-neutral I/O and Windows adapter
├── features/
│   ├── library/        Main shell, shelf, folders, scanner, watcher
│   ├── reader/         PDF reader, outline, page controls, bookmark, ink
│   ├── bookmarks/      Bookmark-management contracts
│   ├── command_center/ Global Ctrl+K search
│   ├── sync/           Diff preview UI
│   └── settings/       Settings surfaces
├── database.dart       Drift tables and repository operations
├── pdf_engine.dart     PDF outlines, annotations, metadata integrity
├── pdf_safe_file_writer.dart
└── sync_engine.dart    PDF ↔ SQLite reconciliation
```

Library discovery runs in bounded concurrent batches. Each finished batch is
committed without removing paths the scanner has not reached; only the final
reconciliation removes missing files and matches safe renames. PDF reads,
writes, validation, and ink indexing run away from the UI isolate where
practical.

### Run checks

```powershell
flutter pub get
flutter analyze
flutter test
flutter test integration_test/windows_reader_workflow_test.dart -d windows
flutter build windows --release
```

### Local CodeGraph knowledge base

The repository includes project-local Codex MCP configuration and an
`AGENTS.md` marker section for
[CodeGraph](https://colbymchenry.github.io/codegraph/getting-started/quickstart/).
CodeGraph supports Dart and resolves symbols, callers, callees, and impact from
a local SQLite graph before an agent falls back to repeated text searches.

Install the CLI once, then initialize this checkout:

```powershell
npm install --global @colbymchenry/codegraph
codegraph telemetry off
codegraph init
codegraph status
```

Restart Codex after installation so `.codex/config.toml` loads the MCP server.
Codex must trust this project before project-local MCP configuration is active.
The generated `.codegraph/codegraph.db` is intentionally ignored by Git: it
contains machine-local paths, stays on the local computer, and is reproducible
with `codegraph init`. Commit the `.codex/config.toml`, `AGENTS.md`, and any
future `codegraph.json` configuration changes, but never the local database.

Useful commands:

```powershell
codegraph sync
codegraph index
codegraph explore "how do PDF ink saves preserve the outline?"
codegraph query PdfEngine
codegraph callers saveEditedPdfRevision
codegraph impact LibraryFolderManager
```

The graph for this checkout currently contains 85 indexed files, 1,749 symbols,
and 4,007 relationships. It is machine-local and can be rebuilt in seconds.

### Lean GitHub automation

Only automation that directly reduces local work and log noise is enabled:

- `.github/workflows/test.yml` runs formatting, analysis, tests, and produces a
  coverage artifact on pushes and pull requests.
- `.github/workflows/build-windows.yml` builds and zips the complete Windows
  runtime bundle for beta tags or a manual dispatch. Flutter, Pub, and native
  Windows build caches reduce repeated compilation time.
- Project-local GitHub MCP configuration uses GitHub's hosted **read-only**
  endpoint, allowing agents to request focused repository, diff, and CI data.

To activate GitHub MCP, create a least-privilege token, store it in the user
environment (never in this repository), then restart Codex:

```powershell
[Environment]::SetEnvironmentVariable('GITHUB_PAT_TOKEN', 'YOUR_TOKEN', 'User')
```

After restarting, use the MCP panel to confirm both `codegraph` and `github`.
The repository ignores `.env`, keys, certificates, the local CodeGraph database,
and generated Repomix outputs to reduce accidental credential or machine-data
commits.

Greptile and Repomix are intentionally not configured because they duplicate
the local CodeGraph index. Release Drafter is deferred until changes flow
through consistently labelled pull requests. External review, refactoring,
dependency, secret-scanning, and documentation apps can be added later when
their permissions and signal justify the extra maintenance.

Run the opt-in personal-file audit on copies only:

```powershell
$env:ATLAS_AUDIT_ARABIC_PDF = 'C:\path\to\arabic-copy.pdf'
$env:ATLAS_AUDIT_ENGLISH_PDF = 'C:\path\to\english-copy.pdf'
flutter test test/runtime_pdf_audit_test.dart
```

### Localization

Translations live in `lib/l10n/app_en.arb` and `lib/l10n/app_ar.arb`.

```powershell
flutter gen-l10n
```

New interactive tree rows should reuse
`lib/widgets/accessible_bookmark_tile.dart` so keyboard traversal, focus, and
screen-reader semantics remain consistent.

## Versioning and release policy

Atlas follows [Semantic Versioning](https://semver.org/):

- `0.x` means storage contracts and APIs can still change before stable `1.0`.
- `0.8.0-beta.1` is the first consolidated beta of library/search, portable
  bookmarks, and PDF reading/ink.
- A new beta suffix is a compatible hardening build; a new minor version adds a
  material core capability.
- Flutter build metadata (`+2`) identifies the packaged build and increases for
  each distributable beta build.

The version is defined in `pubspec.yaml`. Release changes belong in
`CHANGELOG.md`, and engineering scope and acceptance criteria belong in
`PLAN.md`. Delivery order, evidence, and owner stop gates belong in
`CHECKPOINTS.md`.

## Documentation

- [Engineering and accessibility plan](PLAN.md)
- [Delivery checkpoints and owner tests](CHECKPOINTS.md)
- [Release history](CHANGELOG.md)
- [Additional project notes](INFO.md)

## License

Atlas Reader is open-source software released under the [MIT License](LICENSE).
