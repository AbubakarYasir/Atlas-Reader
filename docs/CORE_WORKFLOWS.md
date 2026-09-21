# Core Workflows and Edge-Case Contract

**Priority:** Index / Library → Reader → Bookmarks / Outlines

**Status:** product requirements. N0 does not implement these behaviors.

## Product rule

> **Portable when possible, local when necessary, never lost silently.**

A PDF is the preferred portable authority for outlines and standard annotations when it can be modified safely and legitimately. An Atlas-local overlay is first-class when it cannot.

Atlas never bypasses document security, silently damages signed/certified integrity, overwrites a newer external revision, or claims local data was embedded when it was not.

## Document capability states

Opening and mutation are separate capabilities.

| State | Read | Embed bookmark | Local research | Required behavior |
|---|---:|---:|---:|---|
| Normal writable | Yes | Yes | Yes | Prefer portable outline |
| Open-password, password absent/wrong | No | No | Existing indexed state | Locked; prompt on open |
| Open-password, authorized | Yes | Depends | Yes | Continue permission check |
| Permission-restricted | Yes | If permitted/authorized | Yes | Explain; local-only option |
| Filesystem read-only | Yes | No | Yes | Local mode; Save Copy |
| Sharing-locked | Usually current session | Retry later | Yes | Preserve pending work |
| Signed/certified | Usually | Only if explicitly safe | Yes | Default local/working copy |
| Malformed/partially recoverable | Best effort | No unless validation passes | If identity stable | Recovery/read-only mode |
| Unsupported security handler | Limited/no | No | Existing local | Explain; no circumvention |
| Missing/offline source | No | No | Yes | Retain state; do not purge |
| Cloud placeholder unavailable | No | No | Yes | Do not retrieve network data silently |
| Externally changed while open | Current session | Block | Yes | Reconcile first |

### Password rules

- Distinguish document-open password from permissions/owner authorization.
- Never store passwords in SQLite, logs, exports, filenames, or analytics.
- Session-memory reuse may be allowed pre-1.0.
- Choosing local-only mode suppresses repetitive prompts during the session.
- Never crack/remove/bypass security.
- Later authorization promotes local work only through a previewed merge.

## Bookmark storage states

Every bookmark can report one of:

- **Embedded** — represented in current PDF.
- **Local only** — Atlas-only node.
- **Pending embed** — waiting for safe writable source.
- **Local override** — embedded node has Atlas-only rename/move/destination change.
- **Locally hidden** — PDF node suppressed in Atlas but still exists in PDF.
- **Conflict** — source and local state changed incompatibly.

A subtle status indicator/details surface is enough; default reading UI should remain uncluttered.

## Local overlay requirements

The local overlay supports:

- add bookmark/folder;
- rename;
- destination edit;
- move/reparent;
- reorder;
- arbitrary practical depth;
- delete local node;
- locally suppress embedded node;
- search/tags/descriptions;
- undo where appropriate.

It preserves a base snapshot and enough identity/context to reconcile against later source revisions.

## Promotion to PDF

When a source becomes writable/authorized:

1. re-read source;
2. verify identity/fingerprint;
3. compare source outline to overlay base/current state;
4. preview conflicts/destructive changes;
5. choose Embed, Keep local, Save copy, or Resolve;
6. temp-write and validate;
7. reopen/re-index;
8. only then mark state Embedded.

## Bookmark editor requirements

- discover existing outline at deep hierarchy;
- create folders/nodes;
- rename/delete/move/reorder/nest;
- duplicate visible names under different parents;
- Arabic/Urdu/Unicode/combining marks/mixed direction;
- exact page destinations and safe view-position destinations where supported;
- full breadcrumb identity;
- reveal current location;
- exact jumps from outline/bookmarks/global search;
- keyboard/screen-reader complete manipulation.

## Import/export

### Required lossless format

`.atlas-bookmarks.json` is versioned and includes hierarchy, order, destination, descriptions/tags where supported, storage origin, and guarded document identity metadata. It never includes passwords.

### Human/data formats

- Markdown hierarchy with page references;
- CSV with full breadcrumb/page/storage state.

### Export scope

- all bookmarks;
- selected subtree;
- local-only/pending changes.

### Import modes

- append root;
- append under selection;
- merge;
- explicit replace;
- local-only import even on writable PDF;
- preview before destructive commit.

Foxit XML interoperability may be implemented only after real round-trip fixture tests prove hierarchy/order/destinations/Unicode behavior.

## Library/index states

Library entries distinguish at least:

- Available
- Locked/encrypted
- Read-only/restricted
- Offline source
- Missing
- Cloud placeholder
- Unreadable/corrupt
- Changed since index
- Unsupported

### Scanner requirements

- multiple roots;
- overlapping/nested roots without duplicate logical rows;
- Unicode/long paths supported by runtime;
- permission-denied subfolders do not abort root;
- reparse/junction/symlink cycle protection;
- files may disappear/change during scan;
- cancellation/restart never deletes unseen records;
- watcher event storms are coalesced;
- only successfully enumerated roots may reconcile deletions.

### Identity

Path alone is insufficient. Candidate signals include PDF trailer ID, selected hashes, page count, size, metadata, and prior identity history.

- Safe rename/move preserves local research state.
- A copy is normally a distinct library item.
- Ambiguous candidates remain separate or require user resolution.
- Never attach research data to another copy just because filename matches.

## Reader edge cases

The reader remains stable for:

- image-only/scanned PDFs without OCR claims;
- no outline;
- malformed outline with readable pages;
- mixed page sizes;
- rotation/crop/media boxes;
- long documents;
- page labels different from physical indices;
- Arabic/RTL mixed metadata/outlines;
- source moved/removed after open;
- external modification while open;
- permission changes during session.

Physical page index, PDF page label, bookmark destination, and academic page offset are separate concepts.

## Safe-save contract

Before mutation:

1. source exists or relocation is safely resolved;
2. source revision matches opened/reconciled revision;
3. PDF security permits operation;
4. filesystem destination permits operation;
5. signature/certification consequences are acceptable and explicit;
6. storage is plausibly sufficient.

Commit:

1. write temp;
2. open/validate temp;
3. verify preservation invariants;
4. keep/promote recovery backup where appropriate;
5. replace/promote;
6. reopen/re-index;
7. mark committed.

Failures preserving user work include sharing violation, antivirus lock, permission revoked, disk full, destination lost, process crash, source moved, external revision changed, and validation failure.

Recovery choices should be concise: **Retry · Keep local · Save copy · Locate file · Export changes · Discard local changes** when safe.

## Backup/restore

Local-only work makes first-class backup mandatory before Windows 1.0.

Backup protects:

- overlays/tombstones;
- descriptions/tags;
- favorites;
- reading progress;
- roots/configuration;
- safe document identity mapping.

Restore previews unresolved books; filename-only relinking is forbidden.

## Core acceptance fixtures

Eventually maintain copied/test fixtures for:

- writable Arabic/English mixed-outline PDF;
- open-password PDF;
- permissions-restricted PDF;
- filesystem read-only PDF;
- file held by another process;
- signed/certified PDF;
- externally modified document;
- removable/offline root;
- corrupt outline/readable pages;
- image-only PDF;
- duplicate/copy/rename identity cases;
- 10,000-bookmark stress document.

No core checkpoint is complete while any applicable fixture can silently lose research data or mutate the wrong source.