# Atlas Reader Native — Data Model and Format Contracts

This document defines the durable concepts Atlas must own independently of Qt, PDFium, qpdf, SQLite schema details, or any one operating system.

The rule is simple: **engines and databases are implementations; Atlas data semantics are the product contract.**

## 1. Data authorities

Atlas data falls into three classes.

### Portable document-native data

Preferred when safe and supported:

- PDF outline/bookmarks and destinations;
- standard PDF annotations/ink/text markup;
- selected standard document metadata/XMP.

The PDF is authoritative only after a validated commit and reopen.

### Atlas-local document overlay

Used when portable mutation is unavailable, unsafe, intentionally deferred, or conflicted:

- local-only bookmarks;
- pending embedded changes;
- local overrides of embedded nodes;
- locally hidden/tombstoned embedded nodes;
- unresolved conflicts;
- local descriptions/tags not representable safely in the PDF;
- local annotations when embedding is not permitted.

This data must survive application restart, backup/restore, source rename/move reconciliation, and temporary source unavailability.

### Application-local state

- library roots;
- favorites;
- recents;
- reading progress;
- UI/settings/preferences;
- cache metadata;
- migration bookkeeping;
- diagnostics preferences.

Do not disguise application-local state as portable PDF metadata.

## 2. Document identity

A document is not identified by filename or path alone.

Atlas uses a guarded identity model composed from multiple signals, such as:

- current logical storage reference/path;
- filesystem identity where available;
- file size;
- modification/revision information;
- PDF trailer/file identifiers where reliable;
- selected structural fingerprints;
- optional fast content fingerprints;
- user-confirmed relinking history.

No single weak signal may authorize destructive reassociation.

### Rename/move

A high-confidence same-document move may retain app-local research state.

### Copy

A copied PDF normally becomes a distinct document identity even when its bytes initially match. Atlas may offer an explicit relationship/relink workflow but must not silently merge their future local research.

### Ambiguity

When identity is ambiguous, keep records separate and surface a resolution workflow. Never attach one book's local research to another merely because names/pages look similar.

## 3. Revision and mutation identity

Every open session stores enough source revision information to detect external change before mutation.

A mutation transaction must compare the current source with the revision observed when the editable state was derived. Mismatch becomes a conflict/reconciliation state rather than a blind overwrite.

## 4. Page and destination model

Atlas distinguishes:

- zero/one-based internal physical page index as an implementation detail;
- user-visible physical page number;
- PDF page label;
- optional academic/page-offset display;
- precise destination coordinate/zoom mode when supported.

A bookmark destination is not just an integer page number.

Canonical domain destination should be able to represent:

- page identity/index;
- destination kind (`XYZ`, fit page, fit width, etc. where preserved);
- coordinates in normalized Atlas page space;
- optional zoom;
- source-engine raw information only inside adapters when needed for round-trip preservation.

Adapters normalize rotation/crop/media-box differences into Atlas coordinates.

## 5. Bookmark node model

A bookmark node requires stable Atlas identity independent of visible title.

Conceptual fields:

- Atlas node ID;
- document identity;
- parent node ID / root;
- sibling order;
- title;
- destination or grouping-only state;
- description;
- tags;
- source/portable node fingerprint where applicable;
- storage state;
- local revision;
- embedded/source revision observed;
- conflict metadata when unresolved.

Duplicate titles are valid. Path/breadcrumb identity must never use title alone.

### Storage state

The canonical states are:

- `Embedded`;
- `LocalOnly`;
- `PendingEmbed`;
- `LocalOverride`;
- `LocallyHidden`;
- `Conflict`.

Transitions must be explicit and tested. A node does not become `Embedded` until save, validation, reopen, and re-index succeed.

## 6. SQLite policy

SQLite schema may evolve; the domain contract above does not.

Rules:

- schema migrations are monotonic/versioned;
- migration failure never silently resets the profile;
- database writer operations are transactional;
- FTS tables are derived/search structures and can be rebuilt;
- caches are disposable;
- local research overlay, identity links, and migration history are durable and must not be treated as cache;
- WAL/journal strategy is selected with crash/recovery tests;
- backup uses a transactionally consistent snapshot, not arbitrary copying of a live database.

## 7. Atlas bookmark interchange format

Primary bookmark interchange extension:

`.atlas-bookmarks.json`

Requirements:

- UTF-8;
- explicit format name;
- semantic schema version;
- document hint/identity metadata that cannot force auto-binding;
- complete hierarchy/order;
- exact Atlas destination representation;
- titles/descriptions/tags;
- local/embedded provenance when useful;
- export timestamp/tool version as informational metadata;
- unknown future fields ignored/preserved according to version policy;
- import validation before mutation.

Example conceptual envelope:

```json
{
  "format": "atlas-bookmarks",
  "schemaVersion": 1,
  "document": {
    "title": "…",
    "pageCount": 123,
    "identityHints": {}
  },
  "roots": []
}
```

This is illustrative, not a frozen JSON schema. N5 must publish the actual versioned schema and fixtures before declaring import/export stable.

## 8. Markdown and CSV bookmark export

These are human/interchange convenience formats, not lossless backups.

### Markdown

Preserve visible hierarchy and useful destination/page information. It may include tags/descriptions. Import from Markdown is not required for 2.0 unless promoted explicitly.

### CSV

Use stable documented columns and UTF-8. Include breadcrumb/path, title, page label/index information, description/tags where useful. CSV cannot represent every PDF destination semantic losslessly and must not be presented as lossless.

## 9. Atlas backup format

Backup is a versioned Atlas-owned package, logically containing:

- manifest/schema version;
- app-local research state;
- local bookmark overlays/tombstones/conflicts;
- favorites/progress;
- library roots/settings;
- guarded document identity/relink information;
- migration metadata;
- integrity/checksum information.

Source PDFs are excluded by default.

The package format must be documented and testable independently of a live installation. Restore validates schema and package integrity before committing local profile changes.

A future container choice (ZIP or equivalent) is implementation detail and should be selected in N8; inner records stay versioned.

## 10. Flutter migration contract

The Flutter profile is an input format, never modified in place.

Migration pipeline:

1. identify legacy schema/version;
2. read into a neutral migration model;
3. resolve documents conservatively;
4. preview matched/unresolved/skipped records;
5. write native state transactionally;
6. record migration source/version/result;
7. permit safe re-run without duplicate research state.

Do not make the native schema mimic the Flutter schema merely to simplify import.

## 11. Logs and diagnostics

Ordinary logs must not contain:

- passwords;
- PDF page/text content;
- bookmark descriptions/tags by default;
- annotation text by default;
- binary document fragments;
- unnecessary full private paths when a hashed/redacted reference is enough.

Diagnostic bundles need explicit user generation and preview/redaction rules.

## 12. Forward-compatibility rules

For every Atlas-owned persisted/interchange format:

- include a schema version;
- document compatible reader/writer behavior;
- never silently downgrade data;
- preserve unknown fields when the format contract says round-trip preservation is required;
- provide explicit unsupported-newer-version errors;
- fixtures from every released schema version remain in regression tests.

## 13. Data-loss invariant

Any operation that cannot prove a safe transformation must prefer:

**retain local state + surface unresolved status**

over
**guess + overwrite/delete**.

This invariant outranks convenience.