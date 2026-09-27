# Atlas Reader Native — Windows 2.0 Feature Scope

## Product thesis

[Competitive gap actions](COMPETITIVE_GAP_ACTIONS.md) define additional required outcomes, stage owners and acceptance tests. These are planned requirements, not current N3 features.

Atlas Reader 2.0 wins by being exceptionally good at **Library/Index → Reader → Bookmarks**, then adding writing/printing/metadata without weakening those workflows.

It does not need every Acrobat/Foxit feature to be competitive. It must be faster, clearer, safer, more research-oriented, more local-first, and better with Arabic/RTL and deep outlines in the workflows it does choose to own.

Priority labels:

- **P0** — cannot ship Windows 2.0 without it.
- **P1** — important Windows 2.0 feature after the core is dependable.
- **P2** — useful follow-up; may defer if it threatens quality/time.
- **Deferred** — explicitly out of Windows 2.0.

### Offline launch scope decision — 2026-09-27

The owner delegated selection of missing pre-2.0 capabilities, with online features and collaboration explicitly excluded. Foxit Reader is the closer everyday capability reference, not a visual template or a requirement for complete feature parity. Required additions and their acceptance tests are C17–C22 in the competitive action register. P1 launch commitments are required before stable, not silently optional; any removal needs an explicit owner scope decision.

Windows 2.0 has no accounts, sync, shared reviews, cloud connectors, online AI, document uploads, remote fonts/resources, telemetry or in-app update checks. User-initiated ordinary web links may open the system browser after destination confirmation; this is not an Atlas online service. Offline local exports are included and do not imply collaboration infrastructure.

## 1. Library and index — P0

### Sources

- add/remove multiple explicit library roots;
- open individual admitted books outside roots without silently enrolling their parent folder;
- recursive discovery;
- safe overlapping/nested roots;
- removable/external drives;
- unavailable/offline roots preserved rather than purged;
- conservative junction/symlink/reparse-point behavior;
- direct-open documents appear in Recents/Library according to product rules.

### Formats

- PDF: full 2.0 support;
- EPUB: non-DRM reflowable EPUB Library admission and full reading through F1; fixed-layout EPUB remains separately capability-gated;
- CBZ and CBR: image-sequence Library admission and full reading through F2, with CBR blocked until a licensed, secure RAR route passes;
- user-owned, DRM-free AZW3, MOBI and PRC files: Library admission and reading through F3 at the fidelity the qualification corpus proves;
- KFX: mandatory F4 feasibility/ADR decision before 2.0; support is not advertised unless lawful non-DRM fixtures and a maintainable parser pass;
- DRM-protected books remain explicit unsupported states; Atlas does not bypass DRM.

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
- use the current filename stem as the dependable primary Library name; keep the extension as a separate protected format field and show embedded/user-set title and author as secondary metadata;
- metadata editing and filesystem renaming are separate explicit operations; a normal rename edits only the stem and must preserve the admitted extension (`.pdf`, `.epub`, or another supported type);
- sort normal Library, folder and Favorites views by that visible filename rather than by hidden embedded title metadata;
- keep the presentation contract format-neutral for `.pdf`, `.epub`, `.azw3`, `.kfx`, `.mobi`, `.prc`, `.cbr` and `.cbz`, while admitting each format to discovery/opening only after its own implementation and compatibility gates pass;
- treat `docs/FORMAT_EXPANSION_ROADMAP.md` as binding pre-2.0 work while preserving the rule that a format is not advertised until its own checkpoint is Accepted;
- favorites;
- recent/opened state;
- reading progress;
- local collections and book tags with search/filter and backup, without moving source files (N7);
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
- side-by-side views of the same or two documents, independent navigation and explicit active pane, within the shared memory budget (N4);
- simplified text reading view with adjustable size/wrapping for qualified text-bearing PDFs; clearly separate it from exact-layout view, retain source-page navigation, and show honest unavailable/reading-order limitations for scans or unsupported structure (N4; no OCR claim).

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

### Complete annotation control — required before 2.0

Pen and freehand marker/highlighter are separate tools; text highlighting follows selected text when a reliable text layer exists. Users control color, opacity and thickness, with persistent presets and visible selected-tool state. Include stroke erasing and partial erasing of Atlas-created ink, lasso/rectangle and multi-selection, move/resize, copy/paste/duplicate, delete, and undo/redo. Typed notes remain editable. Keyboard commands and accessible alternatives must accompany pointer tools.

Every admitted annotation supports its applicable operations; controls explain when an imported subtype is read-only rather than flattening or discarding it. Group transforms, partial erasing and undo preserve page coordinates, pressure data where supported and untouched marks. For PDF annotations, save locally or embed only through qualified N5 safe-save rules; export/print choices clearly state whether marks are included. Flattened export is an explicit new-copy operation with a warning that editability is lost, never the default save.

N6 acceptance requires a published annotation-type/operation matrix, physical pen/marker/palm tests, mixed Arabic/Urdu text, rotated-page geometry, crash recovery and independent-reader reopen. A toolbar icon alone is not delivery. The owner confirmed the additional offline editing scope; E1–E5 in [Offline editing roadmap](OFFLINE_EDITING_ROADMAP.md) are required before stable 2.0.

### Standard markup

Target standard PDF representations where interoperability is reliable:

- highlight;
- underline;
- strikeout;
- sticky note/comment;
- freehand ink.

N6 also requires typed free-text notes, line/arrow/rectangle/ellipse annotations, move/resize/delete/undo, and a searchable annotation list. Arabic/Urdu text appearance and independent-reader reopening are required. Original PDF text/image editing and equations are required in E2. Full diagramming, snapping and advanced callouts remain outside the explicitly selected scope.

Research notes can be exported locally as versioned lossless Atlas annotation JSON and readable Markdown, including source identity and page/location references. JSON re-import requires preview, duplicate handling and explicit unresolved-source states. Markdown is a readable report, not a lossless import format. This is offline portability, not shared reviews (N6; non-PDF location types extend it in F1–F3).

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

Original PDF text/image editing is required in E2, with qualified operation limits and explicit unsupported-object handling.

### Ordinary PDF form filling — N7 / P1 launch commitment

- existing non-scripted AcroForm text fields, checkboxes, radio buttons and choice lists;
- keyboard navigation, accessible field labels and Arabic/Urdu values/appearance;
- dirty-state indication, cancel/recovery, preview and explicit safe save to a copy by default;
- preserve fields as editable and verify values and appearances in an independent reader;
- enforce permissions, signature protection and external-change checks from N5;
- N7 excludes XFA, JavaScript calculations/validation, submit actions, signature fields and form creation; E3 adds AcroForm authoring and E5 adds offline signatures. Explain unsupported dependencies before accepting input; never pretend a required calculation ran.

N7 must qualify the read/widget/appearance/write route through an ADR and licensed fixtures before implementation. The accepted N2 engines are not assumed to provide a ready-made form UI or appearance writer. If the required route fails, N7 is blocked pending a tested solution or explicit owner scope revision, not silently downgraded.

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

Source books are excluded by default.

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

- Open book picker limited to Accepted formats;
- launch with an Accepted book path;
- Open With;
- optional per-format file associations at installer/user choice;
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
- privacy-safe by default: no book text, passwords, bookmark descriptions, or file contents in ordinary logs;
- optional user-generated diagnostic bundle with preview/redaction policy;
- no telemetry/network dependency in Windows 2.0;
- online update checking and automatic crash uploads are excluded from 2.0; release downloads and developer security checks remain outside the runtime.

## 13. Expanded offline capabilities and exclusions

[Offline editing roadmap](OFFLINE_EDITING_ROADMAP.md) is binding: E1 page organization, E2 original text/image editing and equations, E3 form authoring, E4 secure redaction, E5 offline visible/certificate signatures and evidence-limited verification. These follow F4 and precede N8. They reuse N5 safety and require open-source qualification ADRs and owner PASS. No feature is claimed implemented.

### Excluded from Windows 2.0

- OCR;
- cloud accounts/sync;
- collaborative annotation;
- AI document analysis/chat;
- XFA, document scripts/calculations and network form submission; ordinary AcroForm filling is N7, authoring E3;
- online certificate enrollment, revocation fetching, trusted timestamp requests and hosted e-sign services; offline signatures/qualified verification are E5;
- OCR-backed search and automated document-version comparison;
- full notebook authoring, audio-linked notes, arbitrary toolbar customization and executable plugins; equations are E2;
- office conversion;
- multimedia/3D;
- EPUB/ebook content editing and portable annotation writing beyond each accepted format checkpoint;
- browser-based application shell; a sandboxed document-layout runtime for F1 requires its own ADR and security/license qualification;
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
