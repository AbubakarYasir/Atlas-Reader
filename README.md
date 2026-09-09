# Atlas Reader

Atlas Reader is a private, offline-first tool for organizing the bookmarks in
your PDF books. It keeps a fast local bookmark library on your computer and
can write bookmark changes back into the PDF itself.

That means a bookmark you create in Atlas can also appear in compatible PDF
readers such as Adobe Acrobat. Your books stay your books: Atlas does not need
an account, cloud storage, or an internet connection to manage them.

> **Project status:** Windows desktop application through Stage 11. PDF edits
> use a verified temporary-file replacement workflow; keep independent backups
> of irreplaceable research files as a normal precaution.

## What you can do today

- Select a PDF from your computer.
- Open a PDF in a scrollable, zoomable reader with Basic and Research modes.
- See its existing outline/bookmarks, including chapters, sections, and deeper
  sub-sections.
- Add a bookmark for a page, optional note, and comma-separated tags.
- Search bookmarks across the local Atlas library.
- Press `Ctrl+K` from anywhere in Atlas to search bookmarks and scanned books.
- Use Atlas with a keyboard or screen reader, and switch the app between
  English and Arabic (right-to-left) from **Settings**.
- Add one or more folders to your library; Atlas scans their PDFs and notices
  additions, removals, and renames while the app is open.
- Browse a visual bookshelf, annotate PDF text, compare two books, and keep a
  linked research scratchpad beside your reading.
- Use hands-free auto-advance and a 200–800 WPM speed reader for selectable
  text on a PDF page.
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

### Open a book

1. Start the app.
2. Choose **Select PDF**.
3. Pick a PDF file.
4. Atlas checks the PDF and shows any bookmark changes waiting to be saved.
5. Choose **Open reader** to read the PDF inside Atlas. Use the zoom controls,
   mouse wheel/trackpad, or touch gestures to navigate it.

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
- Open **Settings** → **Library folders** → **Add folder** to add folders that
  contain your PDFs. Atlas scans subfolders too. Choose **Scan now** whenever
  you want an immediate refresh; otherwise it refreshes after it notices a
  PDF being added, changed, renamed, or removed.
- Open **Library files** to see the PDFs Atlas has found. Select a file there
  to begin reading it.
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

### Research and speed-reading tools

Switch to **Research Mode** in the reader to reveal the outline, annotations,
citation, thumbnail, and display controls. Use the play-circle button for
hands-free page advance; its floating controls set direction and reading speed.
Use the speed icon to open the current page in the RSVP speed reader. It works
with selectable PDF text, supports Arabic right-to-left words, and offers 200
to 800 words per minute.

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

## Current limits

- Library indexing reads PDF filenames and outline/bookmark counts. It does not
  upload or modify a PDF merely because it was scanned.
- Rename reconciliation retains local bookmark links when Atlas can safely
  match a renamed PDF within the same library folder by its modification time
  and outline signature. If a file cannot be matched, it is indexed as new.
- EPUB discovery is supported in the library; PDF remains the format currently
  available in the full reader and document-sync workflows.
- Notes and tags are stored locally; PDF outline titles are the portable part.
- The app is currently designed and tested primarily for Windows.

## For developers

### Run checks

```powershell
flutter test
flutter analyze
```

The project currently has 41 test cases, including two opt-in real-file smoke
tests. They cover bookmark-tree paths,
mixed Arabic-English identifiers, snapshot compatibility, sync differences,
Arabic PDF round trips, library data behavior, reader controls, research
workspace state, widget startup, and isolate-backed PDF operations.

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
│   ├── reader/         # PDF reader and bookmark composer
│   ├── bookmarks/      # Bookmark-management contracts
│   ├── command_center/ # Ctrl+K global search overlay
│   ├── research/       # PDF annotations and citation tools
│   ├── speed_reading/  # Hands-free and RSVP reading modes
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

Large document-byte reads and generated-file writes use a background isolate on
Windows. The reader uses a virtualized PDF viewer, which renders pages as they
are needed instead of building every page in the Flutter widget tree at once.

## Roadmap and release notes

The plain-English plan is simple: make bookmark management reliable first, add
an accessible PDF reading experience next, then add library scanning, Android,
Arabic/RTL support, and EPUB.

For the detailed accessibility, architecture, and delivery roadmap, see
[PLAN.md](PLAN.md). For the record of completed work, see [CHANGELOG.md](CHANGELOG.md).
