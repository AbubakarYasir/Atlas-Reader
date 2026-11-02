# ADR-0008 — Bookmark Overlay Storage and Location Envelope

- **Status:** Proposed (implemented in N5.2; becomes Accepted with `N5 PASS`)
- **Date:** 2026-11-03
- **Checkpoint:** N5 — Bookmarks + document safety (`2.0.0-beta.3`), sub-stage N5.2

## Context

Atlas must keep a person's bookmarks when the PDF cannot be changed (protected,
signed, read-only, in use, offline) and must never report local work as being
in the PDF before a validated write ([DATA_AND_SAFETY.md](../DATA_AND_SAFETY.md)).
That needs a durable record of each node's storage state, the PDF outline it
was based on, and destinations that survive restart, zoom, layout and rotation
changes. Later formats (ebooks, comics after 2.0) must fit the same model
without being stored as screen page numbers (C06).

The format is persisted in every profile and will be carried in backups (N8)
and `.atlas-bookmarks.json` (N5.5), so it constrains the future.

## Decision

1. **Same profile database, new tables (schema 5).** The overlay lives in the
   Atlas profile SQLite database beside the library index:
   `bookmark_overlays` (one revision counter per document),
   `bookmark_base_snapshots` and `bookmark_base_nodes` (the PDF outline as last
   read, with the revision stamp it came from), `bookmark_nodes` (Atlas's own
   nodes) and `bookmark_node_tags`. Rows reference `documents`, so research is
   tied to Atlas document identity ([ADR-0005](ADR-0005-document-identity-and-reconciliation.md)),
   never to a path. The library index stays the single owner of migrations; the
   overlay repository opens the same file and requires schema 5 or later.
2. **Storage states are explicit and enforced twice.** `Embedded`, `LocalOnly`,
   `PendingEmbed`, `LocalOverride`, `LocallyHidden`, `Conflict` follow one
   transition table in core (`core/bookmarks/BookmarkOverlay`). The repository
   refuses any write that skips the table and enforces **"no false Embedded"**:
   an Embedded node must equal its base node (title, destination, parent and
   position), new nodes start `LocalOnly` or `PendingEmbed`, and only a verified
   save (N5.4, `EmbedProof`) can take local work to Embedded. Database `CHECK`
   constraints repeat the state/base-key/conflict rules.
3. **Every change is one transaction** with an optimistic revision check
   (`apply(document, expectedRevision, changes)`), WAL journal and
   `synchronous = FULL`. After a crash the overlay is the state before the
   change or after it, never between.
4. **Nodes from the PDF are hidden, not deleted.** Removing a local node keeps
   a tombstone row (`deleted_utc_ms`) for reconciliation and backup; its ID is
   never reused.
5. **Versioned location envelope** stored as text:
   `atlas-location/1;scheme=pdf;page=3;label=iv;kind=xyz;left=0.125;top=0.5;zoom=1.5`.
   The major version and scheme come first; values are percent-encoded UTF-8;
   unknown keys are kept and written back in order; a newer major version or an
   unknown scheme is reported as unsupported and the text is kept unchanged.
   PDF coordinates are fractions of the page as displayed at view rotation 0
   (after its own /Rotate and crop box), origin top-left. Numbers use the
   shortest exact round-trip form, independent of locale.
6. **User text is exact UTF-8.** Titles, notes and tags are validated as UTF-8
   and stored byte for byte; nothing is trimmed, normalized or transliterated.
   Size limits (200,000 nodes, depth 256, 16 KiB titles, 1 MiB notes, 256 tags)
   are high for real research and stop hostile input.

## Alternatives considered

- **A separate overlay database file.** Rejected: backups and restore need one
  consistent snapshot of identity and research, and foreign keys to
  `documents` would be lost.
- **A JSON blob per document.** Rejected: every edit of a 10,000-node tree would
  rewrite the whole blob, partial corruption would lose the document's research,
  and SQL could not enforce state rules.
- **Storing destinations as page numbers, or as engine data.** Rejected: page
  numbers are not destinations, and engine types must not cross the facade
  ([ADR-0004](ADR-0004-pdf-engine-responsibilities.md)).
- **JSON for the location envelope.** Rejected for the stored column: a flat,
  versioned key=value form is enough, has no parser dependency in core, and
  maps losslessly into the N5.5 JSON export.

## Consequences

- Reading the outline, editing and reconciling all go through one repository;
  the UI never emits SQL. Calls run off the UI thread. Indicative only: on a
  Linux development container (g++ 13 `-O2`, SQLite 3.45) a 10,000-node overlay
  loads in about 11 ms and is written in about 130 ms; Windows Release numbers
  are recorded with N5.3.
- Base adoption never overwrites an existing base: a changed outline is
  reported for reconciliation (N5.4) and nothing is applied.
- Schema 5 adds tables only; existing profiles upgrade in one transaction and a
  failed upgrade leaves schema 4 intact.

## Revisit conditions

- Reconciliation (N5.4) or JSON export (N5.5) needs fields the envelope or the
  node row cannot hold without a major version.
- Measured edit latency on Windows for 10,000+ nodes exceeds the N5 budget.
- A non-PDF format checkpoint after 2.0 needs a scheme whose locator cannot be
  expressed as key=value fields.

## Related

[CHECKPOINTS.md](../../CHECKPOINTS.md#n5--bookmarks--document-safety-200-beta3) ·
[DATA_AND_SAFETY.md](../DATA_AND_SAFETY.md) ·
[N5 evidence ledger](../baselines/N5_BOOKMARKS_SAFETY_EVIDENCE.md)
