# Atlas Reader — Document Safety, Data Model and Security

Binding contract for identity, bookmarks, saving, import/export, backup and security. Implemented across N3–N8, qualified in N9–N10.

> **Portable when possible, local when necessary, never lost silently.**

A PDF is the preferred home for outlines and standard annotations **when it can be changed safely and legitimately**. When it can't, Atlas's local overlay is first-class research state, not a failed-save cache. Atlas never bypasses document security, silently damages signed/certified integrity, overwrites a newer external revision, or claims local data was embedded when it wasn't.

## Data model

### Three kinds of data

| Kind | Examples | Authority |
|---|---|---|
| **Portable, in the document** | PDF outline/bookmarks and destinations, standard annotations/ink/markup, selected metadata/XMP | The PDF, but only after a validated write and reopen |
| **Atlas-local overlay** | Local-only bookmarks, pending embeds, local overrides, locally hidden nodes, conflicts, notes/tags that can't go into the PDF, local annotations on protected files | Atlas. Must survive restart, backup/restore, rename/move and offline periods |
| **App state** | Library folders, favorites, recents, progress, settings, cache metadata, migration bookkeeping | Atlas. Never disguised as PDF metadata |

### Document identity

Never by filename or path alone. Guarded identity combines several signals: logical storage reference, file-system identity, size, modification/revision, PDF trailer ID where reliable, structural fingerprints, optional fast content hashes (xxHash is only one signal) and user-confirmed relink history ([ADR-0005](decisions/ADR-0005-document-identity-and-reconciliation.md)).

- **Rename or move** with high confidence keeps research.
- **A copy** becomes a separate document, even with identical bytes; an explicit relink workflow may be offered.
- **Ambiguity** keeps records separate and asks the user. Never attach one book's research to another because names or page counts look alike.

Every open session stores enough revision information to detect an external change before any write. A mismatch becomes a conflict, never an overwrite.

### Pages and destinations

These are distinct: internal page index (implementation detail), the user-visible page number, the PDF page label, the optional academic offset, and the precise destination. A destination is **not** just an integer. It can hold page identity, kind (`XYZ`, Fit, FitH, FitV…), coordinates in normalized Atlas page space and optional zoom. Engine raw data stays inside adapters for round-trip preservation. Adapters normalize rotation and crop/media boxes. Ebook and comic locations (after 2.0) use the same versioned location envelope defined in N5, never transient screen page numbers.

**Location envelope (N5.2, [ADR-0008](decisions/ADR-0008-bookmark-overlay-and-location-envelope.md)):** `atlas-location/1;scheme=pdf;page=3;label=iv;kind=xyz;left=0.125;top=0.5;zoom=1.5`. Page is the zero-based index; the label is informational. Coordinates are fractions (0–1) of the page as displayed at view rotation 0, origin top-left; each kind allows only its own coordinates (FitR needs all four). Values are percent-encoded UTF-8; unknown keys are kept and written back; a newer major version or unknown scheme is kept as raw text, never rewritten or dropped.

### Bookmark node

Stable Atlas ID (independent of title) · document identity · parent and sibling order · title · destination, or grouping-only · description · tags · source fingerprint · storage state · local revision · observed source revision · conflict metadata. Duplicate titles are valid; breadcrumbs never rely on titles alone. Titles, notes and tags may be in **any language or script** and are stored as exact UTF-8; Atlas never transliterates, reorders, normalizes or trims user text when saving (normalization is applied only inside search indexes).

**Storage states:** `Embedded` (in the current PDF) · `LocalOnly` · `PendingEmbed` (waiting for a writable source) · `LocalOverride` (embedded node with a local rename, move or destination change) · `LocallyHidden` (suppressed in Atlas, still in the PDF) · `Conflict`. Transitions are explicit and tested (the table is in `core/bookmarks/BookmarkOverlay.cpp`): editing an `Embedded` node makes it `LocalOverride`; `LocalOnly` ⇄ `PendingEmbed` by request; `Embedded`/`LocalOverride` → `LocallyHidden` and back (to `Embedded` only if unchanged); any state but `Conflict` → `Conflict`; `Conflict` → `LocalOnly` (keep local) or `Embedded` (take the PDF's); hidden and conflicted nodes can't be edited until unhidden or resolved; nodes from the PDF are hidden, never deleted by Atlas alone; removed local nodes leave tombstones. Nothing becomes `Embedded` before save → validate → reopen → re-index succeeds, and an `Embedded` node must always equal the PDF's node in the base snapshot: same title, destination and parent, and Embedded siblings keep the PDF's relative order (local nodes may sit between them). Editing a PDF bookmark back to the PDF's version makes it `Embedded` again. Undo may restore a removed local node from its tombstone, with the same ID, only as an explicit restore.

**Which book an open file is (N5.3):** the Bookmarks panel asks the library for the documents indexed at the file's exact path (ASCII case-insensitive only when nothing matches exactly). Exactly one → its overlay opens for editing. None → the PDF's own outline is shown read-only, with how to add the book to the library. More than one → read-only as well: research is never attached to a guess. The default UI shows these quietly (an icon and text in details), not loudly.

### SQLite policy

Migrations are monotonic and versioned; a failed migration never resets a profile. Writes are transactional. FTS tables and caches are rebuildable; the overlay, identity links and migration history are **durable** and never treated as cache. The WAL/journal strategy is chosen with crash tests. Backups use a consistent snapshot, never a copy of a live file.

## Document capabilities

Opening and changing are separate capabilities.

| State | Read | Embed | Local research | Behavior |
|---|:-:|:-:|:-:|---|
| Normal writable | ✓ | ✓ | ✓ | Prefer the portable outline |
| Open password missing or wrong | ✗ | ✗ | Indexed state | Locked; prompt on open |
| Open password given | ✓ | Depends | ✓ | Continue the permission check |
| Permission-restricted | ✓ | Only if permitted or authorized | ✓ | Explain; local-only option |
| Read-only file system | ✓ | ✗ | ✓ | Local mode; Save copy |
| Sharing-locked | Usually | Retry later | ✓ | Keep pending work |
| Signed/certified | Usually | Only if explicitly safe | ✓ | Default to local or a working copy |
| Malformed but readable | Best effort | Only if validation passes | If identity is stable | Recovery/read-only mode |
| Unsupported security handler | Limited | ✗ | Existing | Explain; never circumvent |
| Missing/offline | ✗ | ✗ | ✓ | Keep state; never purge |
| Cloud placeholder | ✗ | ✗ | ✓ | No silent network fetch |
| Changed externally while open | Current session | Blocked | ✓ | Reconcile first |

**Passwords:** the open password and the permissions/owner password are different concepts. Passwords are held in session memory only (cleared as far as C++/Qt allows); they never reach SQLite, logs, exports, filenames, diagnostics or session restore. Choosing local-only mode stops repeated prompts during the session. Persistent storage would require a future ADR and an OS secure store. No cracking or bypass, ever.

**Preferences → Identity and Security (N5.3):** the name, organization and email the reader types are kept exactly as typed (up to 256 characters each) in the local preferences file, never in PDFs until a later stage writes an author into a note or markup the reader creates, and never in logs or diagnostics. Security options only decide when Atlas re-checks a file and whether the protection label shows; they cannot turn any protection off. The Signatures panel reads signature fields through the N5.1 qpdf check and never claims a signature is valid.

## Local overlay and promotion (N5)

The overlay supports add bookmark/folder, rename, edit destination, move, reorder, any practical depth, delete local node, locally hide an embedded node, notes, tags, search and undo. It keeps a **base snapshot** for later reconciliation.

**Promoting to the PDF** when the source becomes writable or authorized: re-read the source → verify identity → compare the source outline with the overlay base and current state → preview conflicts and destructive changes → choose *Embed · Keep local · Save copy · Resolve* → temporary write and validation → reopen and re-index → only then mark `Embedded`.

## Safe save

**What Save writes (N5.4, [ADR-0009](decisions/ADR-0009-outline-safe-save.md)):** the PDF outline becomes the bookmark tree the reader sees. Local-only, pending and changed bookmarks are written; untouched PDF bookmarks keep every key they had; a PDF bookmark deleted in Atlas stays in the PDF unless the save preview's removal is ticked (its children move up); notes and tags stay in Atlas; a conflict blocks Save. Destinations are converted from Atlas's displayed-page fractions through the page's crop box and /Rotate. Atlas matches each PDF bookmark to its object by place and title and refuses to save if any does not match.

**Preflight** (every write): identity confirmed or safely relocated · revision unchanged · PDF security permits the operation · destination writable · signature/certification consequence acceptable and explicit · enough storage · the operation fits the engine's preservation guarantees.

**Commit:** temporary write → open and validate → verify preservation invariants ([QUALITY.md §3](QUALITY.md#3-preservation-and-interoperability)) → keep a recovery backup → replace → reopen and re-index → mark committed.

**Failures that must preserve work:** sharing violation, antivirus lock, permission revoked, disk full, destination lost, crash, source moved, external change, validation failure. Recovery choices (only the safe ones): *Retry · Keep in Atlas · Save a copy · Locate file · Export changes · Discard changes*.

## Import and export (N5)

- **`.atlas-bookmarks.json`**, lossless and versioned: UTF-8, `format` name, semantic `schemaVersion`, document identity *hints* that can never force automatic binding, full hierarchy and order, exact Atlas destinations, titles, notes, tags, provenance, and informational exporter version and time. Unknown fields are handled by the version policy; newer schemas are explicitly rejected. Never contains passwords. N5 publishes the real JSON Schema and fixtures.
- **Markdown** (readable hierarchy with page references) and **CSV** (UTF-8, documented columns, full breadcrumb, page and storage state) are convenience formats, never called lossless.
- **Scope:** all, a subtree, or local-only/pending changes. **Import modes:** append at root, append under the selection, merge, explicit replace (with preview), import as local-only even into a writable PDF.
- Foxit XML only after real round-trip fixtures prove hierarchy, order, destinations and Unicode.

## Library scanning

Multiple and overlapping folders without duplicate rows · Unicode and long paths · permission-denied subfolders don't abort a folder · junction/symlink cycle protection · files may change mid-scan · cancel or restart never deletes unseen records · watcher event storms are coalesced · **only a successfully enumerated folder may reconcile deletions.**

## Backup and migration (N8)

**Backup package** (versioned, Atlas-owned, documented, testable without a live install): manifest and schema version · overlays, tombstones and conflicts · notes, tags and collections · favorites and progress · folders and settings · identity and relink data · migration metadata · integrity checksums. Source PDFs are excluded by default. Restore validates the package and schema first, previews unresolved books, and never relinks by filename. The container format (ZIP or similar) is chosen in N8; inner records stay versioned.

**Flutter migration:** identify the legacy schema → read into a neutral model → resolve documents conservatively → preview matched, unresolved and skipped items → write the native state in a transaction → record source, version and result → safe to re-run without duplicates. The legacy profile is input only and is never modified. Don't shape the native schema after Flutter's. Embedded PDF outlines and annotations come across naturally through indexing.

## Format rules for every Atlas-owned file

Schema version always · documented compatible reader/writer behavior · never silently downgrade · preserve unknown fields when the contract requires a round trip · clear errors for unsupported newer versions · fixtures from every released schema stay in regression tests.

## Security model

All PDFs, paths, imported bundles, legacy databases, metadata and external file changes are **untrusted input**.

**Principles:** least privilege and no admin rights · no runtime network dependency · never execute document content · never build shell commands from filenames · parsers isolated behind Atlas interfaces · writing is more privileged than reading · keep user data on failure · passwords stay ephemeral · logs are privacy-minimized.

| Surface | Risks | Guards |
|---|---|---|
| PDF parsing/rendering | Malformed graphs, huge dimensions, decompression bombs, recursion, hostile metadata, parser memory bugs | Pinned engines, typed contained errors, limits where Atlas allocates, never write a recovery-mode PDF without validation, replaceable engines, monitor advisories. Out-of-process engine only if evidence justifies it |
| Paths and files | Traversal, cycles, command injection | Paths are data; cycle protection; no following links outside folders without a policy; controlled temporary/backup sibling names |
| Imported JSON/backups | Resource abuse, malformed IDs | Validate schema, size, depth, node count, string sizes, page ranges and duplicate IDs; import never executes links |
| Legacy migration | Corrupt or hostile data | Read-only source, parameterized SQL, version detection, unresolved instead of guessed |
| Links and actions | Launch, script, remote fetch | No JavaScript; launch/file actions refused or confirmed; external URLs open only after an explicit click with the destination shown; unusual schemes blocked |
| Signed/certified PDFs | Silent invalidation | Detect; local or working copy by default; never claim validity Atlas can't verify (cryptographic validation comes after 2.0) |

**Native hardening:** high warnings, toolchain security defaults (ASLR, DEP, CFG), RAII and standard containers, checked integer conversions around coordinates and counts, `std::span`, fuzzing Atlas parsers, sanitizers and CodeQL. Don't disable mitigations without evidence.

**Resource limits** (high enough for real research, backed by stress tests): concurrent renders, cache sizes, import depth and node count, thumbnail size, decompressed buffers, task queues, log size and rotation, SQLite batch size.

**Network and privacy in 2.0:** no telemetry, uploads, remote indexing, cloud AI, crash upload or update checks. Document scripts and form submission are disabled. Developer tools and CI never touch user documents at runtime.

**Logs and diagnostics:** ordinary logs may include version, typed error category, timings and sanitized diagnostics. They exclude page text, bookmark and annotation text, passwords, binary fragments and full private paths when a redacted reference works. Diagnostic bundles are generated by the user, previewed and redacted.

**Dependencies:** each has a pinned version, upstream source, license inventory, an advisory-monitoring path, a rollback plan and an SBOM entry ([DEPENDENCIES.md](DEPENDENCIES.md)).

**Security tests before RC:** malformed corpus never crashes the app or causes an unsafe save · huge/deep imports are contained · odd filenames stay data · external revision blocks overwrite · passwords absent from logs, DB and exports · restricted PDFs gain no forbidden changes · signed originals stay unmodified in local-only flow · junction cycles terminate · a malformed backup can't corrupt the profile · interrupted writes leave a recoverable source.

Vulnerability reporting: [SECURITY.md](../SECURITY.md).

## Core acceptance fixtures

Writable mixed Arabic/English outline · open-password · permission-restricted · read-only copy · file held by another process · signed/certified · externally modified · removable/offline folder · corrupt outline with readable pages · image-only · duplicate/copy/rename identity cases · 10,000-bookmark document. No core stage is complete while any applicable fixture can silently lose research or change the wrong file.

> **Invariant:** if a transformation can't be proved safe, keep the local state and show it as unresolved. Never guess and overwrite. This outranks convenience.
