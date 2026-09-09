# Atlas Reader

Atlas Reader is a private, offline-first tool for organizing the bookmarks in
your PDF books. It keeps a fast local bookmark library on your computer and
can write bookmark changes back into the PDF itself.

That means a bookmark you create in Atlas can also appear in compatible PDF
readers such as Adobe Acrobat. Your books stay your books: Atlas does not need
an account, cloud storage, or an internet connection to manage them.

> **Project status:** This is a Windows-focused proof of concept. Use it with
> copies of important PDFs while it is being developed.

## What you can do today

- Select a PDF from your computer.
- See its existing outline/bookmarks, including chapters, sections, and deeper
  sub-sections.
- Add a bookmark for a page, optional note, and comma-separated tags.
- Search bookmarks across the local Atlas library.
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

### Add a bookmark

1. Select a PDF.
2. Enter a clear **Bookmark Title**.
3. Enter the PDF page number, starting at `1`.
4. Optionally add a note and tags such as `study, important`.
5. Choose **Inject Bookmark**.

Atlas immediately stores the bookmark in its local library, then attempts to
write it into the PDF. The status message tells you whether the PDF update
worked.

### Import bookmarks from a PDF

Use **Sync File** when the PDF was changed in Acrobat or another reader, or
when you open a PDF that already contains bookmarks. Atlas imports newly found
outline entries, preserving their full chapter/section hierarchy.

### Save local edits into the PDF

When the **Sync Preview** shows pending additions or removals, choose
**Commit Changes**. Atlas shows what will change, then rebuilds the PDF outline
from the local bookmark tree.

### Search and organize

- Use **Search all bookmarks** to find saved bookmarks by title.
- Change **Group by** to browse by book, tag, or a flat list.
- Expand a book or section to reveal sub-bookmarks.
- Use the edit and delete icons beside a bookmark for local changes.
- In the **By book** view, expand a book and select **Remove all bookmarks for
  this book** to clear that book from the local Atlas list.

Removing a book from the local list does not immediately alter the PDF. A PDF
changes only after **Commit Changes**.

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

- Atlas manages PDF bookmarks; it is not yet a full PDF page reader.
- EPUB files are planned, but not supported yet.
- Notes and tags are stored locally; PDF outline titles are the portable part.
- The app is currently designed and tested primarily for Windows.

## For developers

### Run checks

```powershell
flutter test
flutter analyze
```

The project currently has 15 automated tests. They cover bookmark-tree paths,
snapshot compatibility, sync differences, widget startup, and Windows document
file operations.

### Project layout

```text
lib/
├── core/
│   ├── database/       # SQLite connection infrastructure
│   └── file_system/    # Platform-neutral document I/O + Windows adapter
├── features/
│   ├── library/        # Current workspace and PDF selection
│   ├── reader/         # Bookmark composer; future PDF reading UI
│   ├── bookmarks/      # Bookmark-management contracts
│   ├── command_center/ # Global search UI
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

## Roadmap and release notes

The plain-English plan is simple: make bookmark management reliable first, add
an accessible PDF reading experience next, then add library scanning, Android,
Arabic/RTL support, and EPUB.

For the detailed accessibility, architecture, and delivery roadmap, see
[PLAN.md](PLAN.md). For the record of completed work, see [CHANGELOG.md](CHANGELOG.md).
