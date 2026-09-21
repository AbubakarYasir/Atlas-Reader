# Atlas Reader Native — Windows 2.0 Feature Scope

## Product thesis

Atlas Reader 2.0 wins by being exceptionally good at **Library/Index → Reader → Bookmarks**, then adding writing/printing/metadata without weakening those workflows.

It does not need every Acrobat/Foxit feature to be competitive. It must be faster, clearer, safer, more research-oriented, more local-first, and better with Arabic/RTL and deep outlines in the workflows it does choose to own.

Priority labels:

- **P0** — cannot ship Windows 2.0 without it.
- **P1** — important Windows 2.0 feature after the core is dependable.
- **P2** — useful follow-up; may defer if it threatens quality/time.
- **Deferred** — explicitly out of Windows 2.0.

## 1. Library and index — P0

### Sources

- add/remove multiple explicit library roots;
- open individual PDFs outside roots without silently enrolling their parent folder;
- recursive discovery;
- safe overlapping/nested roots;
- removable/external drives;
- unavailable/offline roots preserved rather than purged;
- conservative junction/symlink/reparse-point behavior;
- direct-open documents appear in Recents/Library according to product rules.

### Formats

- PDF: full 2.0 support;
- EPUB: discovery/metadata only if retained from legacy behavior; full EPUB reading remains deferred unless explicitly promoted.

### Index state

Each book can represent:

- available;
- locked/encrypted;
- read-only/restricted;
- signed/certified;
- offline source;
- missing;
- cloud placeholder/not local;
- unreadable/corrupt;
- changed since last index;
- unsupported.

### Identity

- guarded identity stronger than path alone;
- safe rename/move preserves research state;
- copied books remain distinct unless explicitly reconciled;
- no destructive reassociation based on filename alone;
- ambiguous identity requires separation/user resolution.

### Metadata/search

- title, author, filename, path, dates, page count when available;
- favorites;
- recent/opened state;
- reading progress;
- bookmark text/breadcrumb/tags/descriptions in FTS;
- Arabic/Unicode search normalization policy documented and tested;
- fast filtering/sorting;
- global Command Center (`Ctrl+K`) for book/bookmark navigation.

### Views

- cover grid;
- detailed list;
- compact list;
- folder explorer;
- Recents;
- Favorites;
- Bookmarks;
- unavailable/error state is visible and actionable without breaking the library.

## 2. Reader — P0

### Rendering/navigation

- continuous scroll;
- single-page/page mode where useful;
- fit width / fit page;
- free zoom including under-fit values and high zoom range;
- exact page jump;
- page labels distinct from physical page indices;
- optional academic page offset;
- rotation/mixed page sizes/crop boxes;
- smooth wheel/trackpad/touch navigation;
- multiple documents/tabs with bounded resource retention;
- configurable tab restoration off by default unless owner later changes it.

### Virtualization/performance

- Atlas-owned viewport, not an eager all-page widget;
- high-resolution textures only for visible/near-visible pages;
- bounded thumbnail/page caches;
- cancellation/reprioritization when scroll position changes;
- no render/parse/save on UI thread;
- large-document memory remains bounded.

### Navigation panel

- Outline;
- Pages/thumbnails;
- Bookmarks;
- Annotations once N6 exists;
- panel search;
- selected-page synchronization;
- exact jumps;
- compact-window alternative.

### Text/search/links

Where supported by the PDF:

- text selection;
- copy;
- document text search;
- next/previous result;
- links;
- search result navigation.

Image-only PDF: reading works; Atlas states that a searchable text layer is unavailable. OCR is not implied.

### Reader preferences

- theme/application appearance;
- document brightness/reading appearance if implementable without destructive mutation;
- margin/crop viewing behavior;
- per-document reader state persisted locally;
- keyboard shortcuts.

## 3. Bookmarks and outlines — P0

### Read/navigation

- unlimited practical nesting depth;
- duplicate visible names under different parents;
- Arabic/Urdu/mixed-direction Unicode;
- full breadcrumbs;
- exact page destinations;
- reveal current location/current section;
- expand/collapse operations;
- global search.

### Complete editing

- create bookmark;
- create folder/group where Atlas model requires it;
- rename;
- delete;
- edit destination;
- move/reparent;
- reorder siblings;
- nest/unnest;
- multi-selection/batch operations if achievable cleanly (P1 unless needed for usability);
- undo before commit where practical.

### Storage states

- Embedded;
- Local only;
- Pending embed;
- Local override;
- Locally hidden;
- Conflict.

### Restricted/non-writable behavior

- open password and permissions password are distinct;
- no password/restriction bypass;
- password session-memory handling only unless a later secure-storage ADR explicitly changes it;
- read-only/restricted/signed/externally locked PDFs remain bookmarkable locally;
- Save working/writable copy;
- retry embed when capability becomes available;
- signed/certified original is not silently invalidated;
- external changes block blind overwrite and open reconciliation.

### Portability

Windows 2.0 includes:

- versioned lossless `.atlas-bookmarks.json`;
- Markdown export;
- CSV export;
- all/subtree/local-only export;
- Atlas JSON import with preview;
- append/merge/replace modes;
- import as local-only;
- no compatibility claim with Foxit XML until real fixture round-trips prove it.

## 4. Ink and standard annotations — P1

### Low-latency ink

- Pen;
- Highlighter;
- pressure where hardware/Qt input supports it reliably;
- thickness/color presets;
- eraser;
- select/delete stroke;
- undo/redo;
- page isolation;
- pan/zoom while in writing mode without accidental strokes;
- live stroke rendered independently of PDF serialization.

### Standard markup

Target standard PDF representations where interoperability is reliable:

- highlight;
- underline;
- strikeout;
- sticky note/comment;
- freehand ink.

Advanced shapes/callouts/free-text are P2 unless they are inexpensive after the annotation model is proven.

### Capability model

Annotations reuse the bookmark safe-save/capability/conflict system. Atlas never reports a local annotation as embedded until the PDF mutation validates.

## 5. Printing — P1

- Windows printer selection;
- preview;
- all/current/custom ranges;
- page labels/ranges handled correctly;
- copies/collation;
- orientation;
- paper size;
- fit/actual/custom scale;
- odd/even/reverse where platform path supports it cleanly;
- grayscale;
- multiple pages per sheet if reliable;
- source PDF hash unchanged by print/cancel.

Booklet and advanced prepress features are P2 unless implementation is straightforward and testable.

## 6. Covers and library details — P1

- PDF page-one generated cover fallback;
- embedded/available cover use where reliable;
- cache invalidation based on guarded file revision;
- no stale cover attached to wrong document;
- deterministic fallback for encrypted/offline/unreadable books;
- useful details panel with portable vs app-local values distinguished.

## 7. Metadata — P1

- read document info/XMP where supported;
- clear distinction between portable and Atlas-local fields;
- edit selected portable fields only after N5 safe-save system exists;
- before/after preview;
- preserve unknown XMP namespaces and unrelated document structures;
- explicit Save into PDF;
- cancellation does not mutate source.

Arbitrary PDF object/text editing is Deferred.

## 8. Backup, recovery, and migration — P0 before stable 2.0

### Backup

Versioned Atlas backup includes app-local data needed to recover:

- local-only bookmark overlay/tombstones;
- descriptions/tags;
- favorites;
- progress;
- registered folders;
- settings that materially affect research workflow;
- guarded document identity information.

Source PDFs are excluded by default.

### Restore

- preview;
- schema/version validation;
- unresolved documents shown explicitly;
- no filename-only auto-binding;
- repeatable and testable.

### Flutter migration

- read-only import from legacy profile/database;
- never modifies legacy DB in place;
- preview counts/matches/unresolved data;
- re-running migration is idempotent or clearly versioned;
- PDFs remain the strongest portable bridge.

## 9. Arabic/RTL and accessibility — P0 cross-cutting

- English + Arabic UI resources;
- correct directionality and mirrored layout;
- bidi isolation for paths/mixed text;
- Arabic/English/Urdu bookmark/title fixtures;
- keyboard-only access for every core operation;
- Narrator/UI Automation semantics;
- visible focus;
- 200% UI/text scaling;
- high contrast/system contrast support;
- no status by color alone;
- reduced motion where animations exist;
- meaningful accessible names for custom canvas/tree controls.

## 10. Windows integration — P1/P0 where needed

- Open PDF picker;
- launch with PDF path;
- Open With;
- optional file association at installer/user choice;
- context menus;
- drag/drop P1;
- clipboard;
- reliable filesystem watching;
- long/Unicode paths;
- power/session-close handling that cannot silently lose dirty local work.

## 11. Settings — P1

Keep settings small and organized:

- Library & Storage;
- Appearance & Language;
- Reader & Writing;
- Accessibility & Keyboard;
- Data, Backup & Safety;
- About/Licenses/Diagnostics.

Per-document controls belong in the reader when immediate visual feedback matters.

## 12. Diagnostics/privacy — P0 safety

- local structured logs with rotation/size bound;
- privacy-safe by default: no PDF page text, passwords, bookmark descriptions, or file contents in ordinary logs;
- optional user-generated diagnostic bundle with preview/redaction policy;
- no telemetry/network dependency in Windows 2.0;
- update checking is Deferred unless made explicit and privacy-safe.

## 13. Deferred from Windows 2.0

- OCR;
- cloud accounts/sync;
- collaborative annotation;
- AI document analysis/chat;
- arbitrary PDF text/object editing;
- form authoring;
- signature creation/certificate management workflows;
- office conversion;
- multimedia/3D;
- full EPUB rendering/annotation;
- browser engine/web app runtime;
- password cracking/restriction removal.

## 14. Success measures

Stable Windows 2.0 requires measurable evidence, not subjective “feels fast.” Final thresholds live in `docs/PERFORMANCE.md`, but product-level outcomes include:

- no known core workflow that silently loses research data;
- no blind overwrite of externally changed PDFs;
- no destructive purge of unavailable library roots;
- all P0 scenarios have automated/manual fixture evidence;
- large libraries remain usable during progressive scan;
- large documents remain navigable with bounded memory;
- bookmark search/navigation at 10,000 nodes meets performance target;
- Arabic/RTL/Narrator matrix has no untested core path;
- migration/backup restore is recoverable and documented;
- another standard PDF reader agrees with Atlas on embedded portable data for accepted round-trip fixtures.