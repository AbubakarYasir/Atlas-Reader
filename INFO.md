# Atlas Reader - Project Documentation

**Status:** Phase 1 - Foundation & POC (Proof of Concept)  
**Last Updated:** June 5, 2026  
**Target Platforms:** Android & Windows  

---

## I. Project Vision

**Atlas Reader** is a decentralized research and reading ecosystem built on a **Universal Embedding Protocol (UEP)** where the document itself becomes the database. Unlike traditional e-readers like Librera or Kindle that trap bookmarks in proprietary databases, Atlas Reader embeds bookmarks, notes, and metadata directly into the file itself—making them universal, portable, and visible across any compatible reader.

### The Core Philosophy
**The document is the database.** Your intellectual work is not lost when you uninstall an app or switch devices.

---

## II. Why Atlas Reader?

### Problems with Existing Solutions
- **Librera:** Bookmarks trapped in app-only database. No export to file metadata.
- **Kindle/Google Books:** Completely proprietary. No control over your data.
- **Standard PDF Readers:** No cross-document search for bookmarks.
- **Web Readers:** Cannot modify file metadata.

### The Atlas Reader Advantage
✅ **Portability:** Bookmarks embedded in file, visible in Adobe Acrobat, web browsers, any PDF reader.  
✅ **Professionalism:** Custom bookmark names appear as real TOC entries in professional software.  
✅ **Discovery:** Global search across your entire library to find bookmarks from months ago.  
✅ **Ownership:** Your data is part of the file, not locked in a proprietary database.  

---

## III. The Four Core Features

### **Feature 1: Universal "Mirror" Bookmarking (Embedded & App-Stored)**

**The Concept:**  
When you create a bookmark, it's saved in TWO places simultaneously (Dual-Layer Save):
- **Local Database (SQLite):** For instant UI feedback and fast searches.
- **File Metadata (PDF/EPUB):** As a permanent standard outline/bookmark.

**How It Works:**
1. User clicks "Bookmark Page 42"
2. App writes entry to local SQLite DB (instant UI update)
3. Background service injects bookmark into PDF Outline dictionary or EPUB toc.ncx
4. User can now open the same PDF in Adobe Acrobat, Chrome, or any reader—bookmark is visible

**Conflict Resolution:**
- Each bookmark receives a hidden UUID in file metadata
- If file opened on different device, UUID prevents duplicate entries
- File modification date is the "source of truth"—if file changes externally, app re-syncs local DB

### **Feature 2: Semantic Custom Naming (Markdown Metadata Enrichment)**

**The Concept:**  
Bookmark names become semantic metadata embedded in the file's internal structure, with support for Markdown formatting.

**How It Works:**
1. User renames bookmark to: `**Critical Evidence** - Page 42`
2. **Title Field:** Standard PDF Outline title updated to readable version
3. **Rich Metadata:** Full Markdown text stored in PDF's XMP metadata stream
4. **Result:** 
   - In Adobe Acrobat (basic): "Critical Evidence - Page 42"
   - In Atlas Reader: Renders with **bold**, bullet points, structured formatting

**Implementation Details:**
- Standard PDF Outlines → plain text only
- Extended metadata → XMP metadata stream or Text Annotations
- EPUB support → Stored in nav.xhtml or toc.ncx with custom attributes

### **Feature 3: Command Center (Global Library Search)**

**The Concept:**  
A Spotlight-style search window that indexes bookmarks and comments across your entire library—not just the current book.

**How It Works:**
1. **Background Indexer:** Scans designated library folders
2. **Extraction:** Pulls all bookmarks, comments, and metadata from every PDF/EPUB
3. **Storage:** Writes to SQLite FTS5 (Full-Text Search) database
4. **Query:** User presses `Ctrl+F` (Windows) or accesses Search (Android)
   - Types: "Quantum Physics"
   - Results show ALL matches across library: 
     - [Book A - Page 40]: "Quantum Physics Overview"
     - [Book C - Page 156]: "Quantum Entanglement Theory"
     - [Book Z - Appendix]: "Quantum Fields Discussion"

**Key Features:**
- Hierarchical view: Can nest bookmarks under folders
- Drag-and-drop organization
- Cross-library discovery in seconds
- Markdown preview in search results

### **Feature 4: Two-Way In-App Editing (Bi-Directional Syncing)**

**The Concept:**  
Editing bookmarks instantly updates both the local database AND the file itself. Changes made externally (in other apps or devices) are detected and imported.

**How It Works:**

**Scenario A: Edit in Atlas Reader**
1. User renames bookmark "Chapter 3" → "Chapter 3: The Crisis"
2. App updates SQLite instantly (UI feedback)
3. Background service injects change into PDF dictionary
4. User opens file in Adobe Acrobat → sees updated name

**Scenario B: Batch Operations**
- Select 10 bookmarks → "Shift Pages by +2"
- App loops through each, updates page numbers, batch-injects into file
- Useful when a new edition of a PDF shifts all page numbers

**Scenario C: Clean-Up / External Sync**
1. User opens file modified on another device
2. App detects new bookmarks in file metadata
3. Prompts: "Found 5 new bookmarks from external source. Import?"
4. "Clean-Up Tool" performs diff algorithm:
   - New in PDF → Import to local DB
   - New in DB → Inject into PDF
   - Conflicts → Preserve file version (external edits have priority)

---

## IV. Technical Architecture

### The Universal Embedding Protocol (UEP)

The **UEP** is the ruleset governing all data flow between app and file:

**Core Rules:**
1. **Dual-Sync Logic:**
   - Every write operation triggers `WRITE_LOCAL_DB` (immediate)
   - Simultaneously triggers `WRITE_FILE_METADATA` (background)

2. **Last-Modified Truth:**
   - File hash/modification date is the source of truth
   - External changes override local DB conflicts
   - Ensures consistency across devices

3. **Standardization:**
   - **PDF:** Writes to `Catalog/Outlines` dictionary, XMP metadata for extended fields
   - **EPUB:** Writes to `toc.ncx` or `nav.xhtml` inside container

### Technology Stack

#### Frontend & State Management
| Component | Technology | Rationale |
|-----------|-----------|-----------|
| **Framework** | Flutter (Dart) | Cross-platform, native performance on Android & Windows |
| **State Management** | Riverpod + Freezed | Type-safe, reactive state handling for complex syncing |
| **UI Framework** | Material 3 + Cupertino | Responsive design for desktop (Windows) and mobile (Android) |

#### Backend & Data
| Component | Technology | Rationale |
|-----------|-----------|-----------|
| **Local Database** | SQLite via Drift ORM | Native query support, FTS5 for global search, cross-platform |
| **Full-Text Search** | SQLite FTS5 | Near-instant search across thousands of bookmarks |
| **Background Tasks** | Flutter Isolates | Parallel processing without blocking UI |

#### PDF/EPUB Engines
| Component | Technology | Rationale |
|-----------|-----------|-----------|
| **PDF Engine** | MuPDF (C++ via Dart FFI) | Direct binary manipulation, incremental save for speed |
| **EPUB Engine** | Dart Archive Library | EPUB is ZIP-based, no C++ needed, pure Dart |
| **File I/O** | Platform Channels + SAF | Android scoped storage support, Windows standard I/O |

#### Platform-Specific Abstractions
| Platform | File Access | Implementation |
|----------|-------------|-----------------|
| **Windows** | Standard paths + `dart:io` | Direct file read/write, automatic change detection via Directory.watch() |
| **Android** | SAF (Storage Access Framework) | ContentResolver for URI-based access, cached temp files for updates |

---

## V. Development Phases & Roadmap

### **Phase 1: Foundation & Engine (Current - Weeks 1-6)**
**Status:** In Progress

**Objectives:**
- [x] Set up Flutter project for Android & Windows
- [ ] Integrate PDF engine (MuPDF) via Dart FFI
- [ ] Create File Abstraction Layer (FAL) for platform differences
- [ ] Build basic PDF viewer with page rendering
- [ ] Implement Windows file I/O + Android SAF integration
- [ ] Create test project for PDF metadata injection

**Deliverables:**
- Flutter project skeleton with platform support
- PDF viewer capable of opening and rendering files
- Proof-of-concept for PDF outline injection (write-test to validate)
- Platform-specific file access working on both OS

**Current State:**
- ✅ Flutter project structure created (windows/ & lib/ directories present)
- ✅ pubspec.yaml configured
- ✅ pdf_engine.dart started
- 🔄 MuPDF/PDFium integration in progress

---

### **Phase 2: Universal Embedding Protocol & Core Sync (Weeks 7-12)**

**Objectives:**
- [ ] Implement Dual-Layer Save mechanism
- [ ] Build SQLite schema with Drift ORM
- [ ] Create MetadataManager for file injection
- [ ] Implement UUID-based conflict resolution
- [ ] Add bookmark creation UI
- [ ] Test: Open modified PDF in Adobe Acrobat to verify bookmarks

**Deliverables:**
- Working Dual-Layer Save system
- Bookmark creation fully functional
- Cross-reader compatibility test results
- Feature 1 complete

---

### **Phase 3: EPUB Support & Command Center (Weeks 13-18)**

**Objectives:**
- [ ] Implement EPUB unzip/XML parsing
- [ ] Build Background Library Scanner isolate
- [ ] Create SQLite FTS5 index
- [ ] Build Command Center UI (Windows: overlay, Android: sheet)
- [ ] Implement search algorithm
- [ ] Test global search performance with 1000+ bookmarks

**Deliverables:**
- EPUB bookmark injection working
- Background indexer scanning library folders
- Command Center fully searchable
- Features 1 & 3 complete

---

### **Phase 4: Polish & Advanced Features (Weeks 19-24)**

**Objectives:**
- [ ] Implement Markdown rendering for bookmark comments
- [ ] Build Batch Editing multi-select UI
- [ ] Implement "Shift Pages" offset logic
- [ ] Create "Clean-Up Tool" diff algorithm
- [ ] Windows: Multi-window support
- [ ] Extensive cross-device testing

**Deliverables:**
- All four features complete
- Markdown comment rendering
- Batch operations fully functional
- Cross-device sync verified (Android → Windows → Adobe)
- Beta-ready application

---

## VI. Current Implementation Status

### ✅ Completed
- Flutter project initialized for Android & Windows
- Project structure established
- pubspec.yaml with dependencies
- Basic main.dart scaffold

### 🔄 In Progress
- PDF engine integration (MuPDF FFI bindings)
- pdf_engine.dart implementation

### ⏳ Not Started
- Drift ORM setup
- SQLite schema design
- PDF outline injection logic
- Background scanner
- UI implementation
- Command Center
- Batch editing
- Markdown support
- Cross-device testing

---

## VII. Key Technical Challenges & Solutions

### Challenge 1: PDF Binary Manipulation
**Problem:** Modifying PDF internal structure without corrupting file  
**Solution:** 
- Use MuPDF's incremental save feature (writes only new bytes)
- Implement "safe save": write to `.tmp` file first, verify, then atomic replace
- Version control of PDF: track file hash to detect external changes

### Challenge 2: Android Scoped Storage
**Problem:** Cannot directly write to user's Downloads/Documents folder  
**Solution:**
- Use SAF (`ACTION_OPEN_DOCUMENT_TREE`) to get persistent permissions
- Read/write via Android `ContentResolver` with URI-based access
- Cache temp files in app's private cache directory

### Challenge 3: Cross-Device Conflict Resolution
**Problem:** Same file edited on multiple devices without network sync  
**Solution:**
- UUID-based bookmark identification (hidden in PDF metadata)
- File modification date as conflict resolution point
- "Last-Modified Wins" protocol with user-override options

### Challenge 4: Performance with Large Libraries
**Problem:** Scanning 1000+ PDFs with embedded bookmarks takes time  
**Solution:**
- Background indexing isolate (doesn't block UI)
- Incremental indexing (only rescan files modified since last scan)
- SQLite FTS5 for near-instant global search

---

## VIII. Data Models

### Bookmark Entity (SQLite Schema)
```
bookmarks (
  id: UUID PRIMARY KEY,
  file_path: String,
  page_number: Integer,
  title: String,
  markdown_content: String,
  custom_folder_id: UUID FOREIGN KEY,
  created_at: DateTime,
  modified_at: DateTime,
  is_synced_to_file: Boolean,
  file_hash: String  -- detect external changes
)
```

### Library Index (FTS5 Table)
```
bookmarks_fts (
  id: UUID,
  title TEXT,
  markdown_content TEXT,
  file_name TEXT,
  indexed_at: DateTime
)
```

---

## IX. File Format Integration

### PDF Implementation
- **Storage:** Catalog/Outlines dictionary (standard PDF structure)
- **Extended:** XMP metadata stream for Markdown + rich content
- **Comments:** Embedded PDF Text Annotations
- **UUID Location:** Custom XMP namespace (`atlas:bookmark_id`)

### EPUB Implementation
- **Storage:** `toc.ncx` or `nav.xhtml` inside EPUB package
- **Extended:** Custom XML attributes in manifest
- **Validation:** Ensure EPUB spec compliance for compatibility

---

## X. Success Criteria

### Phase 1 Success
- [ ] PDF viewer renders files without crashes
- [ ] File I/O abstraction works on both Windows & Android
- [ ] MuPDF FFI can read PDF outline structure
- [ ] Can write test data to PDF without corruption

### Phase 2 Success
- [ ] Create bookmark → appears in SQLite + PDF
- [ ] Open PDF in Adobe Acrobat → bookmark visible
- [ ] Rename bookmark → both DB and file updated
- [ ] UUID conflict resolution prevents duplicates

### Phase 3 Success
- [ ] Background scanner indexes 100+ PDFs in <5 seconds
- [ ] FTS5 search returns results in <100ms
- [ ] Command Center UI responds instantly to user input

### Phase 4 Success
- [ ] Markdown bookmarks render correctly
- [ ] Batch edit 50 bookmarks at once
- [ ] Open file on Android, move to Windows via USB, all bookmarks preserved
- [ ] Clean-Up tool merges external bookmarks correctly

---

## XI. Licensing Considerations

**⚠️ Important:** Choose PDF engine carefully for distribution.

- **MuPDF:** AGPL (requires commercial license for closed-source apps)
- **PDFium:** Apache 2.0 (safe for proprietary apps)
- **Poppler:** GPL with exceptions (check terms for your use case)

**Recommendation:** Use PDFium for Google Play Store / Windows Store distribution to avoid licensing complications.

---

## XII. Next Immediate Steps

1. **Integrate PDF Engine:**
   - Set up MuPDF/PDFium C++ wrapper
   - Write basic "open PDF and read outlines" function
   - Test with sample PDF files

2. **Build Drift ORM:**
   - Define SQLite schema for bookmarks table
   - Create data models with Freezed

3. **Create File Abstraction Layer:**
   - Windows: dart:io file operations
   - Android: SAF + ContentResolver integration

4. **Implement First Bookmark Creation:**
   - Basic UI button to add bookmark
   - Write to local DB + file metadata
   - Verify in Adobe Acrobat

---

## XIII. References & Resources

- [Flutter FFI Documentation](https://dart.dev/guides/libraries/c-interop)
- [MuPDF C API](https://mupdf.com/docs)
- [Drift ORM Documentation](https://drift.simonbinder.eu/)
- [SQLite FTS5](https://www.sqlite.org/fts5.html)
- [Android Storage Access Framework](https://developer.android.com/guide/topics/providers/document-provider)
- [PDF Specification - Outlines](https://www.adobe.io/content/dam/udp/assets/open/pdf/spec/PDF32000_2008.pdf)
- [EPUB Specification](https://www.w3.org/publishing/epub32/)

---

**For questions or updates to this documentation, refer to the development team's phase status checklist.**
