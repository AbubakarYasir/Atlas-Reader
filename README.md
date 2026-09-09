# Atlas Reader

Atlas Reader is a private, offline-first Windows reading app built around three
jobs: finding books, jumping through deep bookmarks, and writing directly on
PDF pages. It keeps a fast local index while saving portable bookmarks and ink
back into the PDF itself.

That means a bookmark you create in Atlas can also appear in compatible PDF
readers such as Adobe Acrobat. Your books stay your books: Atlas does not need
an account, cloud storage, or an internet connection to manage them.

> **Project status:** the focused library, bookmark, and PDF ink workflow is
> implemented and under Windows release verification. PDF edits use a verified
> temporary-file replacement workflow; keep independent backups of
> irreplaceable files as a normal precaution.

## What you can do today

- Add one or more library folders containing PDF and EPUB books.
- Search instantly by title, author, or file name and sort by title, author,
  date added, last opened, or reading progress.
- Switch between Cover Grid, Detailed List, and Compact List.
- Open a PDF in a fast, scrollable, zoomable reader.
- See its existing outline/bookmarks, including chapters, sections, and deeper
  sub-sections.
- Add a bookmark for a page, optional note, and comma-separated tags.
- Search bookmarks across the local Atlas library.
- Press `Ctrl+K` from anywhere in Atlas to search bookmarks and scanned books.
- Use Atlas with a keyboard or screen reader, and switch the app between
  English and Arabic (right-to-left) from **Settings**.
- Draw pressure-sensitive freehand pen strokes and translucent highlights,
  choose standard colors and sizes, erase, and undo/redo.
- Save writing as standard PDF `/Ink` annotations so it remains visible in
  compatible external PDF readers.
- Browse bookmarks by book, tag, or a flat indented list.
- Edit or delete individual bookmarks, or remove all local bookmarks for a
  book.
- Import bookmarks already stored in a PDF with **Sync File**.
- Review pending changes and save them into the PDF with **Commit Changes**.

## Windows 11: install and run

### 1. Install the tools once

1. Install [Git for Windows](https://git-scm.com/download/win).
2. Install the [Flutter SDK for Windows](https://docs.flutter.dev/get-started/install/windows/desktop).
3. Open PowerShell and confirm Flutter is ready:

   ```powershell
   flutter doctor
   ```

   Follow any instructions shown by Flutter, especially for Visual Studio's
   **Desktop development with C++** workload, which is needed to run Windows
   Flutter applications.

### 2. Download the project

In PowerShell, choose where you keep code and run:

```powershell
git clone git@github.com:AbubakarYasir/Atlas-Reader.git
cd Atlas-Reader\atlas_poc
```

If you do not use SSH keys with GitHub, download the repository from GitHub as
a ZIP file, extract it, then open PowerShell in the extracted `atlas_poc`
folder instead.

### 3. Install app dependencies

```powershell
flutter pub get
```

### 4. Start Atlas Reader

```powershell
flutter run -d windows
```

The first build can take a few minutes. Later launches are normally faster.
Leave the PowerShell window open while the app is running; press `q` there to
stop it.

### 5. Optional: build a standalone Windows app

```powershell
flutter build windows
```

Then run:

```powershell
.\build\windows\x64\runner\Release\atlas_poc.exe
```

## Everyday use

### Build and search your library

1. Start the app.
2. Open **Settings** → **Library folders** → **Add folder**.
3. Choose one or more folders containing PDFs or EPUBs. Subfolders are scanned
   automatically in small background batches.
4. Open **Library files**. Type in the search box to filter by title, author,
   or file name; use the view and sort controls beside it.
5. Select a PDF to read it. EPUBs are indexed and searchable today, but the
   full reader, bookmark embedding, and ink workflow currently require PDF.

### Add a bookmark

1. Select a PDF.
2. Enter a clear **Bookmark Title**.
3. Enter the PDF page number, starting at `1`.
4. Optionally add a note and tags such as `study, important`.
5. Choose **Inject Bookmark**.

Atlas immediately stores the bookmark in its local library, then attempts to
write it into the PDF. The status message tells you whether the PDF update
worked.

### Bookmark while reading

1. Select a PDF and choose **Open reader**.
2. Scroll to the page you want.
3. Select the bookmark icon in the top-right corner, or press `Ctrl+B`.
4. Enter a title, optional note, and optional tags, then save.

Atlas fills the page number from the page currently visible in the reader, so
you do not need to enter it manually.

### Write or highlight on a PDF

1. Open a PDF and select **Write**.
2. Choose **Pen** or **Highlighter**, then choose a color and thickness.
3. Draw directly on the active page. While a drawing tool is selected, a drag
   writes instead of scrolling; switch to **Select** to navigate normally.
4. Use **Area eraser** to cut away marks, or select a whole stroke and press
   `Delete`. Use `Ctrl+Z` and `Ctrl+Shift+Z` for undo and redo.
5. Select **Save** or press `Ctrl+S`. Atlas validates a temporary PDF before it
   replaces the original and refuses the save if chapters or metadata changed.

Each stroke is stored only on the page where it was drawn. Atlas translates
the display position into native PDF points, including the PDF bottom-left
coordinate origin, so the saved writing stays aligned in other PDF readers.

### Import bookmarks from a PDF

Use **Sync File** when the PDF was changed in Acrobat or another reader, or
when you open a PDF that already contains bookmarks. Atlas imports newly found
outline entries, preserving their full chapter/section hierarchy.

### Save local edits into the PDF

When the **Sync Preview** shows pending additions or removals, choose
**Commit Changes**. Atlas shows what will change, then rebuilds the PDF outline
from the local bookmark tree.

### Search and organize

- Press `Ctrl+K` from anywhere in the app to open the **Command Center**. Its
  results show the book, chapter/section path, and page number. Select a result
  to open that PDF at the saved page.
- Open **Settings** → **Library folders** → **Add folder** to add PDF/EPUB
  folders. Choose **Scan now** for an immediate refresh; Atlas also watches for
  additions, changes, renames, and removals while it is open.
- In **Library files**, choose Cover Grid, Detailed List, or Compact List and
  sort by title, author, date added, last opened, or reading progress.
- Change **Group by** to browse by book, tag, or a flat list.
- Expand a book or section to reveal sub-bookmarks.
- Use the edit and delete icons beside a bookmark for local changes.
- In the **By book** view, expand a book and select **Remove all bookmarks for
  this book** to clear that book from the local Atlas list.

Removing a book from the local list does not immediately alter the PDF. A PDF
changes only after **Commit Changes**.

### Keyboard and Arabic support

- Press `Ctrl+K` to open the Command Center, `Ctrl+B` to bookmark the active
  reader page, and `Ctrl+S` to commit pending bookmark changes.
- In the saved-bookmarks outline, use `Up`/`Down` to move, `Right`/`Left` to
  expand or collapse a branch, `Enter` to select it, `F2` to edit it, and
  `Delete` to remove it.
- Open **Settings** → **Language** and choose **Arabic** for an RTL interface.
  Arabic titles and mixed Arabic-English bookmark names retain their original
  Unicode text in the tree and Command Center.

Atlas announces major save, sync, and error outcomes to Windows screen readers
without moving your keyboard focus.

## Keeping your PDFs safe

Atlas does not overwrite a PDF directly. Before replacing a file, it:

1. Validates the newly generated PDF.
2. Writes it to a temporary `.pdf.tmp` file.
3. Opens and validates that temporary file again.
4. Moves the original aside as `.pdf.atlas-backup`.
5. Promotes the validated temporary file, then removes the backup.

If the final replacement fails, Atlas restores the original where possible. If
you see a message about a recovery file, do not delete it until you have checked
your original PDF.

## Back up Atlas data

The portable information is in your book files, so back up the PDFs themselves
after saving bookmarks or writing. Atlas's fast local library index is stored
in `atlas_db.sqlite` inside the Windows Documents directory used by the app.
To make a complete backup while Atlas is closed:

1. Copy your library folders, including every edited PDF.
2. Search your Documents folder for `atlas_db.sqlite` and copy that file too.
3. If a `.pdf.atlas-backup` recovery file exists, keep it until you confirm the
   matching PDF opens correctly.

## Current limits

- Library indexing reads PDF filenames and outline/bookmark counts. It does not
  upload or modify a PDF merely because it was scanned.
- Rename reconciliation retains local bookmark links when Atlas can safely
  match a renamed PDF within the same library folder by its modification time
  and outline signature. If a file cannot be matched, it is indexed as new.
- EPUB discovery is supported in the library; PDF remains the format currently
  available in the full reader and document-sync workflows.
- Bookmark titles and hierarchy are portable in the PDF outline. Descriptions
  and tags remain in the local index; freehand writing is portable `/Ink`.
- The app is currently designed and tested primarily for Windows.

## For developers

### Run checks

```powershell
flutter test
flutter analyze
```

The suite includes opt-in real-file smoke tests plus deterministic coverage for
bookmark paths, mixed Arabic-English identifiers, snapshot compatibility, sync
differences, Arabic PDF round trips, library search/sorting, native PDF ink
coordinates, page isolation, outline/metadata preservation, widget startup,
and isolate-backed PDF operations.

To run the optional, read-only audit against personal Arabic and English PDF
copies, set the paths only for the current PowerShell session:

```powershell
$env:ATLAS_AUDIT_ARABIC_PDF = 'C:\path\to\arabic-copy.pdf'
$env:ATLAS_AUDIT_ENGLISH_PDF = 'C:\path\to\english-copy.pdf'
flutter test test/runtime_pdf_audit_test.dart
```

### Localization and accessibility

User-facing translations live in `lib/l10n/app_en.arb` and
`lib/l10n/app_ar.arb`. After changing them, run:

```powershell
flutter gen-l10n
```

Reusable tree rows live in `lib/widgets/accessible_bookmark_tile.dart`. They
centralize focus traversal, outline shortcuts, and screen-reader semantics;
new bookmark-tree UI should use them rather than raw `ListTile` widgets.

### Project layout

```text
lib/
├── core/
│   ├── database/       # SQLite connection infrastructure
│   └── file_system/    # Platform-neutral document I/O + Windows adapter
├── features/
│   ├── library/        # Workspace, folder manager, scanner, watcher, PDF list
│   ├── reader/         # Fast PDF reader, outline search, bookmarks, PDF ink
│   ├── bookmarks/      # Bookmark-management contracts
│   ├── command_center/ # Ctrl+K global search overlay
│   ├── sync/           # Sync preview UI
│   └── settings/       # Settings entry point
├── pdf_engine.dart     # PDF outline extraction and writing
├── sync_engine.dart    # PDF/database reconciliation
└── database.dart       # Drift models and repository operations
```

All document reads, writes, renames, and deletions go through
`DocumentFileSystem`. `WindowsDocumentFileSystem` is the current adapter. An
Android Storage Access Framework adapter can implement the same interface
without changing PDF or synchronization logic.

Large document-byte reads, generated-file writes, library discovery, PDF
validation, and ink indexing run away from the UI isolate where practical. The
normal reader is virtualized. Writing uses an isolated `RepaintBoundary` and a
byte-backed editor so the Windows file is not held open during replacement.

## Roadmap and release notes

The active plan is intentionally narrow: perfect library discovery, bookmark
navigation, and responsive PDF writing on Windows; keep platform boundaries
clean for Android later. Text-to-speech, speed-reading, and cloud services are
explicitly deferred.

For the detailed accessibility, architecture, and delivery roadmap, see
[PLAN.md](PLAN.md). For the record of completed work, see [CHANGELOG.md](CHANGELOG.md).
