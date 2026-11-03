# Atlas Reader — Architecture and Technology

How Atlas is built and why. Decisions that constrain the future are recorded as ADRs in [decisions/](decisions/README.md).

## 1. Goals

Predictable frame pacing, low input latency, bounded memory on long PDFs, responsive search and indexing on large libraries, safe document writes, replaceable PDF engines, a credible path to Android/Linux/macOS/iOS, Arabic/RTL and accessibility, and a core that can be tested without starting a GUI.

## 2. Technology (accepted)

| Area | Choice | Record |
|---|---|---|
| Language | C++23 (standard library first: `std::expected`, ranges, chrono, filesystem, `std::span`) | [ADR-0001](decisions/ADR-0001-native-stack.md) |
| UI | Qt 6 + Qt Quick/QML (scene-graph, GPU-backed) | ADR-0001 |
| Windows toolchain | Visual Studio 2022 x64, MSVC v143, Qt **6.10.3** (canonical for CI and release); VS 2026 preset allowed for local development | [ADR-0003](decisions/ADR-0003-n1-qt-toolchain-pin.md) |
| PDF read/render/text/links/outline | Qt PDF 6.10.3 (official, dynamic) behind Atlas facades | [ADR-0004](decisions/ADR-0004-pdf-engine-responsibilities.md) |
| PDF structure/security/writes | qpdf 12.4.1 CLI behind Atlas facades | ADR-0004 |
| Index and search | SQLite 3.53.4 + FTS5, static `x64-windows-static-md` | N3 acceptance |
| Native dependencies | vcpkg manifest mode, pinned baseline `9e2895bf`; Qt installed separately | [ADR-0002](decisions/ADR-0002-vcpkg-manifest.md) |
| Document identity | Atlas-owned IDs, conservative reconciliation | [ADR-0005](decisions/ADR-0005-document-identity-and-reconciliation.md) |
| Build and test | CMake ≥ 3.28 + presets, CTest; Qt Test; Catch2/Google Benchmark only when justified | — |

**Before the release candidate (N10):** move to the current secure, supported Qt LTS/patch that can be reproduced in CI. This needs Debug/Release CI, a benchmark comparison and a license review.

**Considered and not chosen:** Flutter (kept only as a behavior reference), Electron/Tauri/WebView (a browser layer between the pen/page and the GPU), Rust GUI toolkits (higher GUI/mobile risk), .NET/Avalonia (would still lean on native PDF), proprietary PDF SDKs (lock-in), MuPDF (needs a separate licensing decision), standalone PDFium (no official Atlas-controlled build route; kept as comparison evidence).

## 3. Layers

```text
Presentation (QML / Qt Quick)          ← layout, visual state, animation
        ↓ observable state / commands
Application controllers & view models  ← orchestration, cancellation, use cases
        ↓
Portable C++ domain                    ← identity, capabilities, bookmark tree,
        ↓ ports                           overlay, reconciliation, safe-save plan
Infrastructure adapters
 ├─ PDF (Qt PDF read · qpdf structure/write)
 ├─ SQLite / FTS5 repositories
 ├─ filesystem, watcher, picker
 └─ platform services (print, shell, lifecycle, pen)
```

- **Presentation** never decides security, identity, reconciliation or save policy, and never queries SQLite.
- **Application** coordinates open, scan, search, navigate, bookmark, reconcile, save copy and export. It owns task orchestration and cancellation.
- **Domain** is pure C++ values and rules with no Qt GUI types.
- **Infrastructure** implements ports with Qt, the OS and third-party libraries.

Current source layout: `src/core/` (portable contracts: `document`, `index`, `pdf/IPdfEngine.h`, `platform`, `reader`), `src/app/` (controllers, index services, Qt PDF inspector, SQLite index, logging, diagnostics), `qml/` (presentation), `tests/` (index, reader, PDF fixtures).

## 4. Threads and scheduling

The GUI thread owns the Qt Quick scene and UI state. Heavy work runs on **bounded** queues, in priority order:

| Queue | Work |
|---|---|
| interactive | Current-page render, page jump, user search |
| near-view | Neighbor pages, visible thumbnails |
| background | Scanning, covers, metadata refresh |
| serialization | PDF save/validation, export |
| database writer | Ordered, transactional, batched local writes (bookmarks: `BookmarkService`, one worker thread; edits apply in memory at once and their writes queue in order) |
| document text thread | One per open book (`DocumentTextWorker`): outline, search, selection, links, word count, and the Auto Create Bookmarks scan in 40 ms slices so other text work is answered in between; heading rules are pure C++ (`core/bookmarks/HeadingDetector`) |

Cancellation is first-class: a render that scrolled out of view loses priority or is cancelled. Foreground reading always outranks background scanning. No unbounded thread or task creation.

**Never on the GUI thread:** directory traversal, PDF parse/validate/render/save, cover generation, large hashing, large SQL, import/export, tree reconciliation. Target: most GUI callbacks under 2 ms and none over 4 ms ([QUALITY.md](QUALITY.md#5-speed-budgets)).

## 5. Reader viewport (N4)

Atlas owns the viewport; it is not a monolithic PDF widget.

```text
ReaderViewport
 ├─ virtual page layout model   (sizes, rotation, boxes, spreads, direction)
 ├─ render scheduler + texture cache (scale buckets, priorities, budgets)
 ├─ text / search / link overlay
 ├─ annotation overlay          (N6)
 ├─ live ink overlay            (N6)
 └─ selection / interaction overlay
```

- A 2,000-page book never allocates 2,000 full-resolution images. Only visible and near pages get sharp textures; distant pages keep dimensions and optional thumbnails.
- Cache keys include document revision, page, scale bucket, rotation/crop and theme inputs. Don't keep duplicate full-resolution CPU and GPU copies without a measured reason.
- Zoom gestures reuse existing textures and sharpen asynchronously. Fast scrolling may briefly show a placeholder instead of blocking input.
- **Live ink:** pointer samples update a lightweight stroke buffer and scene-graph node. They never touch PDF serialization, SQLite or page re-rasterization. Saving happens after the stroke through the safe-save pipeline.

## 6. PDF facade

```text
IPdfEngine (Atlas-owned)
 ├─ inspect / open        ├─ text / search / links
 ├─ page information      ├─ outline
 ├─ render                ├─ security / capability
 └─ create mutation transaction (qpdf)
```

Adapters normalize coordinates, rotation, crop/media boxes, destinations, permissions and errors into Atlas types. Qt PDF, qpdf or PDFium types never escape. Never call them from QML.

## 7. Files and sessions

Reading and writing are separate capabilities. A reading session may hold immutable bytes or engine handles while the file is moved or locked, without holding unnecessary Windows file handles (proved in N4.1). Every write re-checks identity and revision first, so a stale session can never overwrite a newer external revision.

## 8. Database

SQLite is a local projection and search store, not an authority over portable PDF data. Migrations and schema, a read/search pool, a serialized writer queue, repository interfaces, FTS indexes, identity and availability tables, local bookmark overlay (N5) and app-local state. Scans publish progressively. **Only a successfully enumerated folder may delete records.**

## 9. Platform ports

The core never sees Win32 paths or handles, Android SAF URIs or Apple security-scoped URLs. A logical document reference is resolved by a platform storage adapter. Ports cover file system and storage references, the watcher, picker, printing, stylus/touch, shell/open-with/share, lifecycle/power and, only if a future ADR introduces it, secure credential storage.

## 10. Errors

Errors are typed by meaning, not by library strings: password required · permission restricted · file system read-only · sharing-locked · source offline · external revision conflict · unsupported encryption · malformed but readable · validation failed · storage unavailable. Adapters keep native details for logs; the UI receives localized, actionable states ([DESIGN.md §6](DESIGN.md#6-words-and-states)).

## 11. Drift guards

Do not: call qpdf/PDFium/Qt PDF from QML; put SQL in view models; add Windows path strings to portable bookmark identity; let an engine's object model become Atlas's domain model; render all pages eagerly; create unbounded tasks; turn every object into a `QObject` for convenience; put literal colors or strings in QML screens.

## 12. Changing a decision

Evidence can overturn a decision: reproduce the problem, show why the current boundary can't meet the requirement, compare alternatives, measure migration cost, write or update an ADR, preserve or migrate user data, and re-run the affected gates. Old ADRs are superseded, never deleted.
