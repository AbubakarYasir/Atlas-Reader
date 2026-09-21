# Atlas Reader — Core Workflows, Capability Model, and Edge Cases

**Status:** Product requirements for the road to 1.0. This document defines planned behavior; it does not claim that every requirement is already implemented.

**Priority:** Library/Index → Reader → Bookmarks/Outlines. Safety, accessibility, Arabic/RTL, recoverability, and portability are cross-cutting requirements. Annotation, printing, metadata editing, and later features must not advance by weakening these three core workflows.

## 1. Product contract

Atlas Reader is a local-first research reader. Its core promise is:

> **Portable when possible, local when necessary, never lost silently.**

The PDF is the preferred portable authority for data that belongs in a PDF, such as outlines and standard annotations, **when the document can be modified safely and legitimately**. A local Atlas overlay is a first-class authority when the source cannot or should not be changed.

This refines the earlier shorthand “the document is the database.” That remains the preferred interoperability model, not a reason to fail on read-only, secured, signed, externally locked, or otherwise non-writable books.

Atlas must never bypass PDF security, silently invalidate a digital signature, overwrite a newer external revision, or pretend that app-local data was embedded into the document.

## 2. Core feature priority

### P0 — Reliability foundation

Before any mutation, Atlas determines what the current document actually permits. It preserves user work when the answer changes during a session.

### P1 — Library and index

Discover books, identify them reliably across safe renames/moves, expose availability and capability state, and search metadata/bookmarks without reparsing the whole library.

### P2 — Reader

Open quickly, navigate accurately, remain responsive on large or unusual books, and degrade gracefully when text, metadata, outline, write permission, or the source file itself is unavailable.

### P3 — Bookmarks and outlines

Provide complete hierarchical bookmark management, exact navigation, library-wide search, portable embedding when possible, app-local fallback when necessary, import/export, conflict handling, and recovery.

### After the core

Standard annotations/writing, printing, richer metadata, and other modules follow the checkpoint roadmap. They must reuse the same capability and conflict model instead of inventing separate save rules.

## 3. Document capability model

Opening a PDF and modifying a PDF are separate capabilities. Atlas must preflight both.

| State | Reading | Embedded bookmark changes | Atlas-local bookmarks | Required behavior |
|---|---|---|---|---|
| Normal writable PDF | Yes | Yes | Yes | Prefer portable PDF outline; mirror locally |
| Open-password encrypted, password not supplied | No | No | Existing indexed local data only | Keep library record, show **Locked**, ask for password on open |
| Open-password encrypted, password supplied | Yes | Depends on permissions | Yes | Never persist password by default; continue capability check |
| Permission-restricted PDF | Yes | Only if allowed / authorized | Yes | Explain restriction; offer local-only workflow or permissions password |
| Filesystem read-only file/folder/media | Yes | No | Yes | Work locally; offer **Save writable copy** |
| Temporarily locked by another process | Usually yes from already-loaded bytes | Not until lock clears | Yes | Preserve pending/local work; Retry, Save Copy, or Keep Local |
| Signed/certified PDF | Usually yes | Only if safe and allowed | Yes | Default to non-destructive local mode when mutation can invalidate/violate signature state |
| Malformed/partially recoverable PDF | Best effort | No unless full validation passes | Yes if stable identity exists | Read-only recovery mode; never rewrite a document Atlas cannot validate |
| Unsupported encryption/security handler | No or limited | No | Existing indexed local data only | Explain unsupported security; do not attempt circumvention |
| Source missing/offline/removable drive absent | No | No | Yes | Preserve index and local research data; mark **Offline/Missing**, do not purge blindly |
| Cloud placeholder not locally available | No until available locally | No | Yes | Never trigger network retrieval silently; show availability state |
| Source changed externally while open | Yes from current session copy | Block until reconciled | Yes | Detect conflict before save; never overwrite newer external revision |

### 3.1 Password rules

Atlas distinguishes:

- **Document-open password:** needed to view/decrypt the PDF.
- **Permissions password:** may be needed to alter restricted document content or security-controlled structures.

Rules:

1. Do not store either password in SQLite, logs, crash reports, exported bookmark files, analytics, or filenames.
2. Pre-1.0 may remember a successfully entered password **in memory for the current app session only**.
3. Do not repeatedly prompt during every bookmark operation. Once a user chooses local-only mode for the current document/session, continue locally until they explicitly choose **Embed into PDF** or **Retry**.
4. Never include password-cracking, security-removal, or restriction-bypass behavior.
5. If the user later supplies valid authorization, offer to promote local changes into the PDF through a previewed merge.

### 3.2 Signed and certified documents

A technically writable file is not automatically safe to modify.

Atlas must detect digital-signature/certification state where the PDF engine exposes it. If an edit may invalidate a signature or violate certification permissions:

- show a clear **Signed/Certified — local changes only** state;
- keep bookmarks and research data in the Atlas overlay by default;
- offer **Save working copy** when a user intentionally wants an editable copy;
- never imply that the signed original remains cryptographically unchanged after a mutation.

## 4. Storage and authority model

Each research datum has an explicit origin and authority.

| Data | Preferred portable authority | Local role | Non-writable fallback |
|---|---|---|---|
| PDF outline/bookmarks | PDF outline | Search/index mirror | Atlas overlay becomes authoritative for Atlas-only changes |
| Bookmark title/hierarchy/destination | PDF outline where representable | Mirror + pending edits | Overlay |
| Bookmark description/tags | Portable format when implemented | Current authority | Overlay |
| Standard PDF annotations | PDF annotation objects | Search/index mirror | Local/pending work only if the tool supports a safe fallback |
| Reading progress | — | SQLite | SQLite |
| Favorite state | — | SQLite | SQLite |
| Library membership | — | SQLite/config | SQLite/config |
| Reader view state | — | SQLite | SQLite |

Every bookmark surfaced in the UI must be able to report its storage state:

- **Embedded** — represented in the current PDF.
- **Local only** — exists only in Atlas.
- **Pending embed** — local change waiting for a writable/authorized source.
- **Local override** — an embedded bookmark is renamed/moved/retargeted only inside Atlas.
- **Locally hidden** — an embedded bookmark is suppressed in Atlas but still exists in the PDF.
- **Conflict** — the PDF and local overlay changed incompatibly.

Storage origin must be inspectable without cluttering the default reading UI. A subtle badge/icon plus a details surface is enough.

## 5. Local bookmark overlay

The local overlay is not an emergency cache. It is a supported mode.

It must support the same conceptual operations as a writable outline:

- add bookmark or folder;
- rename;
- edit destination;
- move/reparent;
- reorder siblings;
- nest to arbitrary supported depth;
- delete a local bookmark;
- locally hide an embedded bookmark when the PDF cannot be edited;
- search, tag, describe, favorite/filter where applicable;
- undo before commit where the workflow supports it.

The overlay must preserve enough base information to reconcile against a later PDF revision. A practical model is a base outline snapshot plus local operations/overrides and stable Atlas UUIDs. External PDF nodes cannot be assumed to contain stable IDs, so matching must continue to use hierarchy, destination, order, and guarded heuristics.

### Promotion back into the PDF

When a previously restricted/read-only/locked document becomes safely writable:

1. Re-read and fingerprint the current source.
2. Compare the current PDF outline with the base snapshot and local overlay.
3. Show a diff if the PDF changed externally or if destructive operations are pending.
4. Allow **Embed**, **Keep local**, **Save to copy**, or **Resolve conflicts**.
5. Write using the existing validated temporary-file + backup sequence.
6. Re-read the promoted PDF and only then mark local operations as embedded.

No database status may claim success before the PDF commit validates.

## 6. Bookmark management requirements

### 6.1 Core editing

By 1.0 the bookmark system should support:

- existing PDF outline discovery at unlimited practical depth;
- folders and bookmarks;
- create, rename, delete, move, reparent, reorder, and nest;
- duplicate visible names under different parents;
- Unicode, Arabic, Urdu, combining marks, punctuation, and mixed-direction text;
- page destinations and, where the PDF engine safely supports them, view position/zoom destinations;
- full breadcrumb identity in search results;
- find current bookmark / reveal current location;
- expand/collapse all and persistent expansion state where useful;
- exact-page jumps from Bookmarks, Outline, Library search, and `Ctrl+K`;
- keyboard and screen-reader complete tree manipulation.

### 6.2 Import

Import is always previewable before destructive changes.

Required sources:

1. Existing PDF outline — automatic discovery.
2. Atlas lossless bookmark bundle — required for 1.0.

Import modes:

- append at root;
- append under selected folder;
- merge with current tree;
- replace current local tree only after explicit confirmation;
- import as local-only even when the PDF is writable;
- preview adds, moves, destination changes, deletions, and conflicts before commit.

Potential Foxit bookmark XML interoperability should be investigated with real fixtures, but Atlas must not claim compatibility until round-trip tests prove hierarchy, order, destinations, Unicode, and duplicate names are preserved.

### 6.3 Export

Export must work even if the source PDF is encrypted, restricted, read-only, signed, or temporarily unavailable, as long as Atlas already has the bookmark data.

Required 1.0 exports:

- **Atlas Bookmark Bundle (`.atlas-bookmarks.json`)** — lossless, versioned, hierarchical, includes title, hierarchy, destination, description/tags where present, source/storage state, and guarded document identity metadata. Passwords are never included.
- **Markdown** — human-readable hierarchy with book title, breadcrumbs, and page references.
- **CSV** — flat research/data export with full breadcrumb and page fields.

Useful follow-up:

- HTML export for shareable clickable research outlines;
- tested Foxit XML import/export compatibility;
- library-wide multi-book bookmark export.

Users must be able to export **all bookmarks**, **a selected subtree**, or **local-only changes**.

### 6.4 Conflict behavior

If another application edits the outline while Atlas has local changes, Atlas must not choose a winner silently.

- Non-overlapping additions may be merged automatically only when identity is unambiguous.
- Same-node rename/destination/order conflicts require a preview and user choice.
- **Keep both** is available when meaningful.
- Deletion conflicts show what will disappear from each side.
- Re-running sync after a successful commit must produce zero diff.

## 7. Library and index edge cases

### 7.1 Folder topology

The scanner must safely handle:

- multiple roots;
- nested/overlapping roots without duplicate logical entries;
- Windows path case-insensitivity and Unicode paths;
- long paths supported by the runtime;
- permission-denied subfolders without aborting the whole scan;
- symlinks/junctions/reparse points without recursive cycles;
- files disappearing or changing while a scan is in progress;
- cancellation/restart without deleting unseen records;
- watcher event storms and duplicate events.

Default policy for reparse points should be conservative: never recurse indefinitely and do not silently traverse large external trees.

### 7.2 Availability states

A library record is not simply “present” or “deleted.” Atlas should distinguish:

- Available
- Locked/encrypted
- Read-only/restricted
- Offline source
- Missing
- Cloud placeholder/not local
- Unreadable/corrupt
- Changed since last index
- Unsupported format/version

A temporarily unavailable root, disconnected removable drive, or offline network path must not cause destructive reconciliation. Removal requires confidence that the root itself was successfully enumerated.

### 7.3 Identity, moves, and duplicates

Atlas needs guarded document identity stronger than path alone. Candidate signals may include PDF trailer `/ID`, page count, file size, selected-content hashes, and metadata; no single weak signal should cause destructive reassociation.

Rules:

- A safe rename/move should preserve history, favorites, progress, and local bookmarks.
- A copied PDF is normally a separate library item even when content is identical.
- If two candidates are ambiguous, ask or keep them separate rather than attaching research data to the wrong book.
- Local bookmark bundles may suggest a matching document but must preview remapping when identity is not exact.

### 7.4 Difficult files

Indexing must not fail the whole library because one file is bad.

For encrypted, malformed, metadata-poor, huge, or temporarily locked files:

- keep a record when safe;
- fall back to filename when title/author cannot be read;
- show a per-book state/reason;
- allow retry;
- never fabricate metadata;
- never remove prior good metadata merely because a transient read failed.

## 8. Reader edge cases

The reader should remain useful when the PDF is imperfect.

Required scenarios include:

- scanned/image-only PDF with no text layer — reading and navigation work; text search clearly reports unavailable/no matches; OCR remains out of scope;
- no outline — Pages and user bookmarks still work;
- broken/malformed outline — isolate the outline error rather than crashing the reader;
- mixed page sizes, crop boxes, and rotations;
- very long documents;
- unusual page labels versus physical page indices;
- Arabic/RTL and mixed-script metadata/outlines;
- source renamed/moved while closed — reopen through identity resolution where safe;
- source removed while a tab is open — current byte-backed session remains readable where possible, but save requires a resolved destination;
- external modification while open — saving is blocked until conflict resolution;
- disk/file permission changes during the session — current research work remains recoverable locally.

The page shown to the user, the PDF page index, printed page label, bookmark destination, and academic page offset are distinct concepts and must not be conflated.

## 9. Safe-save and concurrency contract

Before every PDF mutation Atlas performs a save preflight:

1. Source still exists at the expected identity/path or has been safely relocated.
2. Source fingerprint/revision still matches the revision Atlas opened or last reconciled.
3. Current security/permission state allows the requested mutation.
4. Signature/certification policy permits the intended change without misleading the user.
5. Destination is writable and sufficient disk space is plausibly available.

Commit sequence remains:

1. Write temporary file.
2. Open and validate temporary file.
3. Verify required preserved structures/data.
4. Move original to recoverable backup where appropriate.
5. Promote validated file.
6. Reopen/re-index committed result.
7. Only then mark local state as committed.

Failure scenarios that must preserve user work:

- sharing violation / another app holds the file;
- antivirus/indexer transient lock;
- permission revoked between preflight and replacement;
- disk full;
- destination disappeared;
- process crash during temporary write;
- original moved externally;
- external revision changed after preflight;
- validation failure after serialization.

The recovery UI should offer the smallest useful choices: **Retry**, **Keep local**, **Save copy**, **Locate file**, **Export changes**, or **Discard local changes** when genuinely safe.

## 10. Backup and restore

Local-only bookmarks make explicit backup a 1.0 data-safety requirement.

Atlas should provide an **Export Atlas Backup** workflow containing app-local research state and configuration needed for recovery, while excluding source PDFs by default. The format must be documented and versioned.

At minimum it protects:

- local-only bookmark overlays and tombstones;
- descriptions/tags not embedded in PDFs;
- favorites;
- reading progress;
- registered folder configuration;
- enough document identity information to relink safely.

Restore must preview unresolved/missing books rather than attaching data by filename alone.

The existing manual SQLite backup guidance remains useful, but the long-term product should not require ordinary users to locate a database file manually.

## 11. User-facing state and messaging

Do not make restrictions look like failures when Atlas can still work locally.

Preferred wording patterns:

- **This PDF can be read, but it does not currently allow bookmark changes. Atlas can keep your bookmarks locally.**
- **This PDF is signed. Editing the file may affect its signature, so Atlas is keeping your changes local.**
- **The PDF changed in another app. Review the differences before Atlas writes anything.**
- **The source drive is offline. Your Atlas bookmarks and reading history are still available.**

Avoid generic “Save failed” when the app knows the cause.

Password prompts, capability banners, conflict dialogs, export dialogs, and storage-state badges require English and Arabic localization, correct RTL mirroring, keyboard operation, Narrator semantics, 200% text-scale support, and non-color-only status cues.

## 12. Competitive baseline — not a parity checklist

Atlas should learn from established tools without becoming a bloated Acrobat clone.

Observed useful patterns as of September 2026:

- **Adobe Acrobat:** distinguishes document-open and permissions passwords; bookmark modification depends on security permissions; signed/certified PDFs impose editing limitations.
- **Foxit PDF Editor:** complete bookmark tree editing plus bookmark search and selected/all XML import/export; bookmark edits are also security-permission dependent.
- **Librera Reader:** reader-first PDF annotation workflow and the ability to save edited output under a new filename.
- **Xournal++:** strong pen-first annotation model; keeps journal data separately from the background PDF and exports a derived PDF, demonstrating the value of a non-destructive local layer.

Atlas differentiation should remain:

1. library/index, reader, and deep bookmarks as one coherent workflow;
2. local-first and account-free;
3. document-portable when safe;
4. graceful local overlay when not;
5. strong Arabic/RTL and accessibility from architecture onward;
6. explicit conflict/recovery behavior rather than silent mutation.

Reference material:

- Adobe PDF security: https://helpx.adobe.com/acrobat/desktop/protect-documents/protect-with-passwords/security-options.html
- Adobe bookmarks: https://helpx.adobe.com/acrobat/using/page-thumbnails-bookmarks-pdfs.html
- Adobe signed-PDF limitations: https://helpx.adobe.com/acrobat/desktop/e-sign-documents/learn-about-signatures/signed-pdf-limitations.html
- Foxit bookmark management/import/export: https://help.foxit.com/manuals/pdf-editor/mac/en-us/v13/Edit_PDF_Files.html
- Librera PDF annotation/save workflow: https://librera.mobi/faq/annotate-highlight-pdf/
- Xournal++ PDF model: https://xournalpp.github.io/guide/pdfs/

## 13. C2 implementation order

The next bookmark checkpoint should not be implemented as one large UI patch. After C1 is accepted, C2 should proceed internally in this order:

### C2.1 — Capability and local-overlay foundation

- detect/open restricted and read-only states;
- password UX and session-only handling;
- signed/certified safety state;
- local bookmark overlay with Embedded/Local/Pending/Override/Hidden/Conflict status;
- source revision/fingerprint conflict guard;
- Save Copy / Keep Local / Retry flows.

### C2.2 — Complete bookmark editor

- create/edit/delete/move/reorder/nest;
- local suppression/override of embedded nodes;
- duplicate names and deep trees;
- exact navigation and full keyboard/RTL behavior;
- promotion/reconciliation back into a writable PDF.

### C2.3 — Portability and recovery

- Atlas lossless bookmark bundle;
- selected/all/subtree export;
- Markdown and CSV export;
- import preview and merge/replace/append modes;
- local-backup/export path;
- conflict and round-trip tests against external readers.

C2 is not accepted until writable, permission-restricted, filesystem read-only, transiently locked, signed/certified, externally modified, and missing/offline-source fixtures all have explicit tested outcomes.

## 14. Acceptance scenario matrix

The following scenarios become regression fixtures over the roadmap:

| Fixture | Index | Read | Bookmark locally | Embed bookmark | Expected special behavior |
|---|---:|---:|---:|---:|---|
| Normal Arabic/English writable PDF | Yes | Yes | Yes | Yes | Round-trip outline exactly |
| Open-password PDF, wrong/no password | Limited | No | Existing only | No | Locked state, retry password |
| Open-password PDF, correct password | Yes | Yes | Yes | Capability-dependent | Password not persisted |
| Permissions-restricted PDF | Yes | Yes | Yes | No until authorized | Local-only notice |
| Windows read-only file | Yes | Yes | Yes | No | Save Copy option |
| PDF locked by another process | Yes | Yes if loaded | Yes | Retry later | No lost work |
| Signed/certified PDF | Yes | Yes | Yes | Normally local/copy only | Signature warning |
| PDF changed externally after open | Yes | Yes | Yes | Block until reconcile | Conflict preview |
| Removable-drive PDF while drive offline | Preserve | No | Yes | No | Offline, do not purge |
| Corrupt outline but readable pages | Yes | Yes | Yes | Only after safe validation | Isolate outline error |
| Image-only scanned PDF | Yes | Yes | Yes | Yes if writable | No OCR claim |
| 10,000-bookmark stress PDF | Yes | Yes | Yes | Yes if writable | Search/navigation performance gate |

## 15. Fixed boundaries

For 1.0, Atlas does **not** aim to bypass PDF security, crack/remove passwords, perform OCR, edit arbitrary PDF text/objects, support forms/signing workflows, provide cloud accounts/sync, or clone the entire Acrobat/Foxit feature set.

A feature outside the three core priorities should be deferred when it threatens index correctness, reader responsiveness, bookmark reliability, portability, Arabic/RTL, accessibility, or data safety.