# ADR-0009 — Writing Bookmarks into the PDF (Outline Safe Save)

- **Status:** Proposed (implemented from N5.4; becomes Accepted with `N5 PASS`)
- **Date:** 2026-11-03
- **Checkpoint:** N5 — Bookmarks + document safety (`2.0.0-beta.3`), sub-stage N5.4

## Context

N5.4 is the first time Atlas changes a PDF. [DATA_AND_SAFETY.md](../DATA_AND_SAFETY.md)
sets the rules (preflight, temporary write, validation, backup, replace, reopen,
only then Embedded) and [ADR-0004](ADR-0004-pdf-engine-responsibilities.md)
assigns structural writes to the qpdf 12.4.1 CLI. Still open were: *what* Save
writes, *how* the outline is rewritten with qpdf without losing anything the
PDF already had, how Atlas's normalized destinations become PDF coordinates,
and what counts as proof that the write is right.

## Decision

Repair qualification, 2026-11-04: failures are reported as **unchanged**, **saved**,
**restored**, or **recovery required**. These are distinct outcomes; an error after
replacement must not claim the original is unchanged. The implementation and
remaining qualification are tracked in the [closure ledger](../baselines/N1_N5_CLOSURE.md).

1. **What Save writes (Ctrl+S, Quick Access Save).** The PDF's outline becomes
   the bookmark tree the reader sees, in the same order and nesting:
   `LocalOnly`, `PendingEmbed` and `LocalOverride` nodes are written;
   `Embedded` nodes are kept exactly as they are. Notes and tags stay in Atlas
   (a PDF outline has no place for them). A PDF bookmark the reader deleted
   (`LocallyHidden`) **stays in the PDF** unless the save preview's "also
   remove hidden bookmarks from the PDF" is ticked; its visible children then
   move up to the nearest written ancestor. A `Conflict` anywhere blocks Save
   until resolved. Before the first write of a session the preview shows how
   many bookmarks are added, changed, kept and removed. This follows Foxit
   (what you see is what the PDF gets) and PLAN's "portable when possible";
   protected PDFs keep everything local as before.
2. **How the outline is rewritten.** Atlas reads the source with
   `qpdf --json=2` (outline items with their object references, page objects,
   page boxes and rotation, inherited through the page tree) and writes with
   `qpdf --update-from-json`: a fresh set of outline item objects (numbered
   above the file's highest object), a new `/Outlines` dictionary, and
   `/Root /Outlines` pointing at it. Items that already existed keep every key
   they had (`/A`, `/C`, `/F`, `/SE`, …) except the tree links (`/Parent`,
   `/First`, `/Last`, `/Next`, `/Prev`, `/Count`), and the title and destination
   only when the reader changed them. Open/closed state (`/Count` sign) is kept
   for existing items; new items with children start closed. Old outline
   objects become unreferenced and are not written. Encryption is kept (same
   handler and permissions; passwords through standard input only). A PDF base
   item is matched to its object by its place in the tree **and** its title; if
   any does not match, Save is refused (Atlas cannot prove which object it is).
3. **Destinations.** Unchanged destinations are copied untouched. New or changed
   ones are written as `[page /XYZ left top null]` (zoom kept when the envelope
   has one), converted from the envelope's displayed-page fractions through the
   page's crop box (else media box) and `/Rotate` (0, 90, 180, 270). Grouping
   bookmarks without a destination are written without one. An envelope Atlas
   cannot read (newer version, other scheme) blocks Save for that node.
4. **Proof before Embedded.** In order: preflight (fresh capability check:
   outline embedding allowed, not signed/certified, file writable and not in
   use; revision stamp equals the one the overlay was read from; free space);
   write to a temporary file in the same folder; `qpdf --check`; preservation
   check against the source (page count, every page's boxes, rotation,
   resources and annotation count, document info, encryption parameters,
   AcroForm fields, names trees); the new outline read back by **qpdf** equals
   the plan (titles, nesting, order, destination pages and coordinates); the
   new file opened by **Qt PDF** (the engine Atlas reads with, independent of the
   writer) shows the same titles, nesting and pages; a recovery backup of the
   original in the profile (`backups/<document>/`, last three kept); atomic
   replace (Windows `ReplaceFileW`, else rename); the replaced file re-hashed
   and equal to the validated temporary file; then the overlay adopts the new
   outline as its base and the saved nodes become `Embedded` with an
   `EmbedProof`, in one transaction. Before replacement, failures leave the original untouched. After replacement,
   Atlas restores from a validated temporary copy if possible. Failed restoration
   retains both versions and blocks further saves; it does not claim success.

## Recovery journal (schema 1)

Before replacement, an atomically committed `backups/<document>/pending-save.json`
records a random operation ID, document ID, absolute destination path, SHA-256 of
both known versions, backup basename, expected overlay revision and phase (prepared, replaced or committed). Hashes and the database revision, not the phase hint alone, decide recovery.
The backup is hash-verified first. Passwords are never recorded. This is a versioned
sidecar, so no database schema migration is required. A per-document lock prevents
two Atlas save operations from replacing the file concurrently.

On opening the book, and again before saving, recovery compares the actual file and
database with that record:

- Original hash and unchanged overlay revision: clear the interrupted operation.
- Replacement hash and committed next overlay revision with matching source revision:
  recognise the completed save and clear the record.
- Replacement hash and unchanged overlay: copy the verified backup to a temporary
  sibling, recheck the destination, atomically replace, verify, then clear the record.
  The reader asks for a reopen because its in-memory pages may show the other version.
- Any unknown file hash, invalid record, missing/bad backup or unexpected database
  revision: preserve the file, overlay, journal and backup; make the book read-only
  for bookmark editing and explain that explicit recovery is required.

The explicit **Review recovery → Keep current file** action validates that the current
PDF opens, archives hash-verified copies of the original backup and current PDF plus
a resolution receipt in `backups/<document>/resolved-<uuid>/`, rechecks the current
hash and journal, then clears the hold. It never changes the PDF or overlay. It
requires reopening before reconciliation/editing. A bad backup, unreadable current
PDF, failed archive or changed source keeps the hold. Cancel performs no work.
Resolution archives are outside normal retention; failure archives are retained too.

Retention runs only after a conclusive successful save. Pending backups cannot be
pruned by later saves. Process-termination tests cover prepared, replaced,
pre-database-commit and post-database-commit boundaries. These tests do not prove
physical power-loss durability of a disk/controller. Hash checks narrow external
writer races; they are not an operating-system compare-and-swap guarantee against
an uncooperative writer changing bytes at the exact replacement instant.

## Alternatives considered

- **Write only nodes marked "save into the PDF".** Safer by default but unlike
  Foxit and surprising after Ctrl+S; kept as a per-node option instead
  (`Keep in Atlas only` stays available as LocalOnly marking in a later slice).
- **Delete hidden PDF bookmarks on Save.** Matches Foxit, but violates "nodes
  from the PDF are never deleted by Atlas alone"; made an explicit choice.
- **Edit outline items in place** (keep object numbers). Smaller diff, but every
  sibling link and count changes anyway, and qpdf renumbers on write; a fresh
  item set with preserved keys is simpler to verify.
- **PDFium or Qt PDF for writing.** Not qualified for structural writes
  (ADR-0004).
- **Incremental update (append-only).** Would keep the original bytes, but
  qpdf's update-from-json writes a full file; incremental save is reconsidered
  with signed-PDF support (Track F).

## Consequences

- Saving rewrites the file (qpdf full write): unrelated objects are preserved
  structurally, not byte-for-byte; object numbers may change. Signed or
  certified PDFs are never written (their signatures would break).
- A save of a large book costs several qpdf runs (read, write, check, read
  back) on a worker thread; the UI shows progress and stays responsive.
- Recovery backups use profile space; the three ordinary completed-save backups are kept, while pending recovery and explicit-resolution archives are retained.

## Revisit when

- Signed-PDF incremental updates are qualified (Track F).
- The owner prefers "embed only on request" as the default.
- qpdf gains an outline API in the CLI that preserves object numbers.

## Related

[DATA_AND_SAFETY.md](../DATA_AND_SAFETY.md) (safe save, capability table),
[QUALITY.md §3–4](../QUALITY.md), ADR-0004, ADR-0005, ADR-0008, CHECKPOINTS N5.4.

### Combined audit repair (2026-11-04)

Before constructing Qt's eager bookmark model, inspect raw outline links through
the already bundled qpdf process boundary, with a 15-second process timeout,
128 MiB JSON ceiling, 100,000-node and 32-level tree bounds. Repeated references
and malformed links fail closed: pages remain readable and bookmarks stay
unsupported/read-only. No partial tree is adopted as a new authoritative base.
The worker uses a temporary copy of the exact encrypted/source bytes, removes it
on return, and supplies the session password through standard input only.

Explicit Keep current recovery validates the journal identity/path and backup
hash, checks the current PDF, preserves both versions and a receipt in a resolved
subdirectory, rechecks the current file and journal, then clears the pending hold.
Restore original validates the original hash and expected overlay revision,
archives the displaced current PDF, original and recovery record outside ordinary
retention, and uses validated temporary copy plus atomic replacement. Neither
choice edits local bookmarks. Both require closing/reopening before editing.
