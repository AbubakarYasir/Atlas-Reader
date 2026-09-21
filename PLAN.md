# Atlas Reader Native — Master Plan

**Status:** Native successor bootstrap

**Primary implementation target:** Windows 11

**Future platform order:** Android → Linux → macOS → iOS/iPadOS

**Core stack:** C++23 · Qt 6.11.x · Qt Quick/QML · SQLite/FTS5 · PDF-engine abstraction · CMake

## I. North Star

Atlas Reader is a fast, local-first reading and research environment where a user can find a book, get to the exact place, and preserve research navigation without being trapped by a proprietary cloud database.

The native successor exists to make that promise compatible with graphics-heavy PDF workloads and long-term multi-platform deployment.

### Non-negotiable principles

| Principle | Meaning |
|---|---|
| Index → Reader → Bookmarks | Core product work is prioritized in that order |
| Native responsiveness | Expensive work never executes synchronously on the UI/render thread |
| Portable when possible | PDF-native data belongs in the PDF when safe and permitted |
| Local when necessary | Non-writable/unsafe documents retain full research usability through local overlays |
| Never lost silently | No false save success, blind overwrite, or destructive unavailable-root reconciliation |
| Core is portable | Shared domain logic contains no Windows-only assumptions |
| Engine replaceability | PDF renderer/editor is behind interfaces; no vendor library owns product architecture |
| Arabic/RTL first-class | Unicode, bidi, Arabic UI, mixed-script metadata, and outlines are release requirements |
| Accessibility first | Keyboard, screen reader, visible focus, text scale, and non-color status are architectural gates |
| Evidence before claims | Documentation distinguishes planned, implemented, verified, and owner-accepted behavior |

## II. Product pillars

### Pillar 1 — Library and Index

- explicit library roots plus direct-open documents;
- progressive scanning with bounded concurrency;
- guarded document identity across safe moves/renames;
- SQLite/FTS5 search over metadata and bookmarks;
- offline/missing/restricted/unreadable states instead of binary present/deleted;
- no destructive purge when a root cannot be enumerated;
- Arabic/Unicode filenames and metadata;
- responsive library at large scale.

### Pillar 2 — Reader

- GPU-backed virtualized page surface;
- only visible/near-visible pages rendered at useful resolution;
- continuous scroll, page mode, zoom, fit width/page, rotation and crop-aware layout;
- text/search/link layers separated from page texture;
- accurate physical page index versus page label versus academic offset;
- byte-backed sessions where useful to avoid file sharing locks;
- no PDF parse/render/save on the UI thread.

### Pillar 3 — Bookmarks and Outlines

- deep hierarchy and duplicate names under different parents;
- exact destinations and breadcrumb identity;
- complete create/edit/move/reorder/nest/delete workflow;
- global search and command-center navigation;
- embedded PDF outline when safe;
- local overlay for restricted/read-only/signed/offline/conflicted documents;
- import/export and backup;
- previewed reconciliation after external changes.

### Pillar 4 — Standard writing and annotations

After the first three pillars are dependable, add low-latency live ink, highlighting, text markup, notes, and standard portable PDF annotation representations. Live drawing is a GPU/UI overlay first; persistence happens asynchronously.

### Pillar 5 — Platform quality

Windows reaches release quality first. Subsequent platforms adapt storage/input/shell/printing/lifecycle behavior while reusing the core and most QML components.

## III. Architectural constraints

### Pure shared core

The reusable core uses standard C++ types where practical and does not expose Qt GUI, Win32, Java/Kotlin, Swift/Objective-C, or platform path handles in domain interfaces.

Qt may be used in infrastructure/application adapters when it genuinely reduces complexity, but platform-specific code stays behind ports.

### UI boundary

QML is for presentation, interaction composition, and animation. Business rules do not live in JavaScript/QML.

Controllers/view models translate between QML and domain/application services.

### PDF boundary

The product depends on `IPdfEngine`, not on Qt PDF, PDFium, qpdf, or any single library directly.

Likely split:

- rendering/text/search: Qt PDF initially evaluated against PDFium;
- structural transformations: qpdf candidate;
- annotations/outlines/security/capability: Atlas facade over whichever engines pass round-trip tests.

No engine is accepted merely because it renders a sample PDF.

### Storage boundary

SQLite + FTS5 is the planned local searchable store. Database writes are serialized/batched behind a repository boundary. UI code never issues arbitrary SQL.

### Platform boundary

At minimum:

- `IFileSystem`
- file watcher
- file picker
- printing
- stylus/touch input specialization
- shell/open-with integration
- secure credential facility if later needed
- app lifecycle/power events

## IV. Threading and performance model

The GUI/render thread must not perform:

- directory traversal;
- PDF parse or structural validation;
- page rasterization beyond GPU presentation commands;
- cover generation;
- document hashing;
- large SQL queries;
- PDF serialization/save;
- import/export serialization for large trees;
- expensive bookmark reconciliation.

Work is scheduled through bounded worker queues with cancellation and priority.

Reader priority is viewport-first: visible page → neighboring page → requested thumbnail → background prefetch.

See `docs/PERFORMANCE.md` for budgets.

## V. Safety model

Every PDF mutation must re-check:

1. document identity/path;
2. external revision/fingerprint;
3. current filesystem writability;
4. PDF permission/security capability;
5. signed/certified mutation consequences;
6. destination space/availability;
7. preservation invariants.

Commit is temp → validate → backup/replace → reopen → re-index → mark committed.

Local state is never labeled embedded before successful validation.

## VI. Accessibility and internationalization

Release work must include:

- English and Arabic resources from the first user-facing checkpoint;
- RTL layout and bidi isolation;
- mixed Arabic/English/Urdu metadata tests;
- keyboard-only operation;
- screen-reader semantics and announcements;
- visible focus;
- 200% UI scale/text testing;
- no status conveyed by color alone;
- reduced-motion handling where applicable.

## VII. Platform sequence

### Windows

The first complete implementation. Use Qt's Direct3D-backed scene graph by default, plus Win32/Windows APIs only through adapters when Qt cannot provide required behavior.

### Android

Reuse C++ core and QML components; add scoped-storage/document-provider, lifecycle, sharing, and stylus adapters.

### Linux

Reuse desktop shell; qualify Wayland/X11, portals, filesystem/watch behavior, printing, and packaging.

### macOS

Reuse core/desktop shell; add Finder/sandbox/printing/macOS menu conventions and Metal qualification.

### iOS/iPadOS

Reuse mobile shell/core; add document picker/security-scoped file behavior, lifecycle, sharing, and Pencil qualification.

## VIII. Deferred scope

The native migration is not justification to expand scope. Before 1.0, defer unless a checkpoint explicitly changes this:

- OCR;
- cloud accounts/sync;
- PDF password cracking/restriction bypass;
- arbitrary PDF text/object editing;
- forms/signature creation workflows;
- office conversion;
- multimedia/3D;
- AI features;
- EPUB full rendering/annotation.

## IX. Definition of done

A feature is not done until:

- unit/domain tests cover rules;
- integration tests cover engine/platform boundaries;
- release build succeeds;
- relevant benchmark budgets pass;
- Arabic/RTL/accessibility behavior is verified for user-facing changes;
- failure/recovery paths are tested;
- documentation states actual implementation status;
- owner checkpoint is explicitly accepted.

## X. Migration rule

The Flutter application remains a product-behavior reference, not a source-code template. Port behavior and data contracts deliberately; do not recreate Flutter widget structure in C++/QML.

The first native releases may run side-by-side with the Flutter beta. Existing PDFs remain the strongest portable migration path; app-local SQLite migration is introduced only after the native schema is stable enough to avoid repeated destructive converters.