# Atlas Reader

### Less searching. More understanding.

**A fast, local-first PDF reader and research library for people who study deeply. Its interface speaks English, Arabic and Urdu, and everything you write in it can be in any language.**

<!-- atlas-status: N4|in-progress -->

Your next idea might be buried in a thousand-page book or scattered across a hundred PDFs. Atlas brings your books together, gives you a calm place to read, and helps you build a lasting path back to what matters.

Atlas Reader is built and published by **[DerekSol](https://dereksol.com)**. Need custom software, a website or branding? Contact DerekSol at [dereksol.com](https://dereksol.com) or info@dereksol.com.

[The plan](PLAN.md) · [Progress](CHECKPOINTS.md) · [Design](docs/DESIGN.md) · [For developers](docs/README.md)

## What Atlas 2.0 will be

| | |
|---|---|
| **Your books, together** | One searchable library across all your folders and drives. It doesn't lose track of books when you move them or unplug a drive. |
| **Reading with room to think** | A quiet, fast reading workspace with tabs, search, outline and thumbnails. Long books stay smooth. |
| **Bookmarks with depth** | Organize a book into nested research paths, find any bookmark across your whole library, and keep them even when the PDF itself can't be changed. |
| **Your marks on the page** | Pen, highlighter, notes and comments that open in other PDF readers too. |
| **English, Arabic and Urdu interface** | Switch the whole app between English (default), العربية and اردو at any time, with proper right-to-left layouts and Nastaliq for Urdu. |
| **Any language in your notes** | Bookmarks, notes, tags and search accept every script and keep it exactly as you typed it, including mixed Arabic and English. |
| **Yours, offline** | No account, no cloud, no tracking. Your research stays on your computer and can be backed up. |

> Portable when possible. Local when necessary. Never lost silently.

## Where it stands today

| Stage | What it delivers | Status |
|---|---|---|
| Foundation (N0–N2) | Windows app shell, toolchain, PDF engine choice | ✅ Done |
| Library (N3) | Folders, background indexing, search, Favorites, Recents | ✅ Released as [`2.0.0-beta.1`](https://github.com/AbubakarYasir/Atlas-Reader/releases/tag/v2.0.0-beta.1) |
| **Reader (N4)** | **Reading PDFs inside Atlas** | 🟡 **In progress.** Opening, tabs, passwords and page display work. Now: the redesigned workspace in three languages. |
| Bookmarks (N5) | Full bookmark editor, local fallback, import/export | Next |
| Pen and notes (N6) | Ink, highlights, comments | Planned |
| Everyday tools (N7) | Printing, covers, sorting, collections | Planned |
| Backup (N8) | Backup, restore and moving over from the old app | Planned |
| Polish and release (N9–N11) | Language and accessibility qualification, speed, installer, stable **2.0** | Planned |

The current beta is for early evaluation. It has the library, but you can't read inside it yet. No release date has been announced.

**After 2.0:** EPUB, comics (CBZ/CBR), DRM-free Kindle files, PDF editing tools, and later Android, Linux, macOS and iOS. See [what comes after 2.0](docs/AFTER_2_0.md).

## Follow along

Star or watch this repository to follow progress, and see the [change history](CHANGELOG.md) for what's new. Developers can start with the [documentation index](docs/README.md).

---

The earlier Flutter app is no longer developed. It is kept only as a reference on the [`legacy/flutter`](https://github.com/AbubakarYasir/Atlas-Reader/tree/legacy/flutter) branch. The native C++/Qt app lives on `main`.

Atlas Reader's own code is [MIT licensed](LICENSE). Third-party components keep their own licenses; see [dependencies and licensing](docs/DEPENDENCIES.md).
