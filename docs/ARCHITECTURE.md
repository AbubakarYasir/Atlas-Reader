# Architecture

## 1. Goals

Atlas Reader Native is designed for desktop-class PDF workloads while keeping a credible path to Android, Linux, macOS, and iOS.

The architecture optimizes for:

- predictable frame pacing;
- low input latency;
- bounded memory use on long PDFs;
- responsive large-library search/indexing;
- safe document mutation;
- engine replaceability;
- platform portability;
- Arabic/RTL/accessibility;
- testability without booting a GUI.

## 2. Layer model

```text
Presentation (QML / Qt Quick)
        ↓
Application controllers / view models
        ↓
Use cases / services
        ↓
Portable C++ domain
        ↓ ports
Infrastructure adapters
 ├─ PDF engine
 ├─ SQLite/FTS5
 ├─ filesystem/watch
 └─ platform services
```

### Presentation

Owns layout, visual states, interaction composition, and lightweight animation. It consumes observable/view-model state and emits commands. It does not decide save security, reconcile bookmark trees, perform document identity matching, or query SQLite directly.

### Application layer

Coordinates use cases: open book, scan root, search, navigate, add bookmark, reconcile, save copy, export, etc. It owns cancellation/task orchestration but delegates rules to domain services.

### Domain/core

Pure C++ value objects and rules. Examples:

- document identity;
- capability state;
- bookmark tree and local overlay;
- conflict/reconciliation plan;
- library availability state;
- page/destination concepts;
- safe-save decision model.

### Infrastructure

Implements ports using Qt/OS/third-party libraries.

## 3. Process/thread model

The main GUI thread owns Qt Quick scene/UI state. Heavy work uses bounded queues.

Suggested queues:

- **interactive** — current-page render, page jump, user-requested search;
- **near-view** — adjacent page rendering, visible thumbnails;
- **background** — scans, covers, metadata refresh;
- **serialization** — PDF save/validation/export;
- **database writer** — ordered transactional local persistence.

Cancellation is first-class. A page render that scrolls far out of view loses priority/cancels rather than consuming CPU merely because it started first.

## 4. Reader rendering architecture

The reader is not a single monolithic PDF widget. Atlas owns the viewport.

```text
ReaderViewport
 ├─ virtual page layout model
 ├─ page texture cache
 ├─ text/search/link overlay
 ├─ standard annotation overlay
 ├─ live ink overlay
 └─ selection/interaction overlay
```

A 2,000-page document should not allocate 2,000 full-resolution page images. Only visible and near-visible pages get high-priority textures. Distant pages retain dimensions/metadata and optional low-resolution thumbnails.

### Live ink

Stylus movement updates a lightweight stroke buffer and scene-graph node. It does not serialize a PDF, update SQLite, or invalidate the underlying page raster per pointer event.

Persistence happens after the interaction boundary and uses the standard capability/safe-save pipeline.

## 5. PDF engine facade

Application code targets an Atlas facade, conceptually:

```text
IPdfEngine
 ├─ inspect/open
 ├─ page information
 ├─ render
 ├─ text/search/links
 ├─ outline
 ├─ security/capability
 └─ create mutation transaction
```

Rendering and structural transformation may use different underlying libraries. That is intentional.

The engine adapter must normalize page coordinates, rotations, crop/media boxes, destinations, permissions, and errors into Atlas domain types.

## 6. File/session model

Opening and writing are separate capabilities.

A reading session may hold immutable source bytes or engine handles while the filesystem source becomes temporarily locked/moved. Mutation still revalidates the current source identity/revision before commit.

No stale session may overwrite a newer external revision.

## 7. Database architecture

SQLite is a local projection/search store, not permission to destroy portable document state.

Planned separation:

- migrations/schema;
- read/search pool;
- serialized writer/transaction queue;
- repository interfaces;
- FTS indexes;
- document identity/availability tables;
- local bookmark-overlay state;
- app-local favorites/progress/settings.

Library scan batches publish progressively. Only a successfully enumerated root may participate in deletion reconciliation.

## 8. Platform ports

Core code must not know about Win32 paths/handles, Android SAF URIs, or Apple security-scoped URLs.

A logical document reference resolves through platform storage adapters to capabilities/streams/paths appropriate for that OS.

This is necessary because Android/iOS file ownership models differ fundamentally from Windows paths.

## 9. Error model

Errors should be typed by domain meaning rather than raw library strings:

- password required;
- permission restricted;
- filesystem read-only;
- sharing locked;
- source offline;
- external revision conflict;
- unsupported encryption;
- malformed but readable;
- validation failed;
- disk/storage unavailable.

Adapters preserve native diagnostic details for logs while UI receives localized actionable states.

## 10. Testing boundaries

- Domain tests require no Qt GUI.
- Engine contract tests run against copied fixtures.
- Database tests use temporary databases.
- Platform adapter tests isolate filesystem/watch behavior.
- Reader integration tests verify page/render/navigation state.
- Hardware qualification covers pen/touch/HDD/printer separately.

## 11. Rules against architectural drift

Do not:

- call qpdf/PDFium/Qt PDF directly from QML;
- put SQL into view models;
- add Windows-only path strings to portable bookmark identity;
- let an engine's object model become the public Atlas domain model;
- render all pages eagerly;
- use unbounded task/thread creation;
- convert every subsystem object into `QObject` merely for convenience.