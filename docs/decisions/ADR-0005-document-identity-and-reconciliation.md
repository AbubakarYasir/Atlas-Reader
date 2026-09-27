# ADR-0005 — Document Identity and Reconciliation

**Status:** Accepted  
**Date accepted:** 2026-09-27  
**Checkpoint:** N3 — Library and index foundation (`2.0.0-beta.1`)

## Context

A PDF can be renamed, moved, copied, replaced at the same path, temporarily
unavailable with its drive, or missed by an interrupted scan. Atlas must keep a
person's library state attached to the correct book without merging distinct
copies or deleting records merely because storage is unavailable.

Path, filename, title, PDF object number and engine-native handles are not
durable document identity. Content equality alone also cannot distinguish a
move from copy-then-delete.

## Decision

Atlas assigns its own durable document ID and stores locations and observed
file revisions separately.

Automatic relinking is deliberately narrow: Atlas may treat a newly observed
path as a move only when the previous path was proven missing by a successfully
completed scan and the filesystem object identity is preserved. The decision
must be represented as a reviewable reconciliation diff before persistence.

The remaining cases follow these rules:

- when the original location still exists, an identical-looking candidate is a
  distinct copy;
- matching content, PDF identifiers or metadata without preserved filesystem
  identity can propose a relationship but cannot authorize automatic relinking;
- an offline root or incomplete/cancelled/partial scan makes the relationship
  ambiguous, never missing or new by proof;
- a different filesystem object at the same path is ambiguous and cannot take
  the preceding document's identity;
- unavailable, locked, unreadable and unsupported documents keep explicit
  states instead of being silently removed;
- ambiguous decisions remain visible for owner review and preserve both the
  prior record and candidate evidence until resolved.

## Alternatives considered

### Path as identity

Rejected because ordinary rename and move operations would lose favorites,
recents and later research state, while replacement at the same path could
incorrectly inherit another book's identity.

### Filename, title or metadata matching

Rejected as authoritative because these values are neither unique nor stable,
especially in multilingual libraries.

### Content hash or PDF identifier as automatic identity

Rejected as sufficient proof. Copies can be byte-identical and PDF identifiers
can be duplicated or absent. These signals remain useful for an ambiguity
proposal only.

### Purge missing paths after every scan

Rejected because disconnected drives, permission failures, cancellation and
partial scans would destroy valid library state.

## Consequences and tradeoffs

Positive:

- temporary storage failure cannot erase a library;
- a proven rename/move can retain Atlas state;
- copies stay separately manageable;
- uncertain cases are surfaced instead of guessed;
- the policy is independent of Qt PDF, qpdf and any future engine.

Costs:

- Atlas must persist filesystem identity and revision evidence per location;
- cross-filesystem moves will usually require review because filesystem
  identity changes;
- reconciliation needs a proposal/diff/apply workflow and restart tests;
- conservative ambiguity creates occasional owner work in exchange for safety.

## Revisit conditions

Revisit only with evidence that the policy causes unacceptable false matches or
excessive unresolved cases, or when a platform supplies a stronger durable file
identity contract. Any relaxation must retain copy separation, offline-root
safety and reviewable persistence.

## Related checkpoints and documents

- `CHECKPOINTS.md` — N3.3 identity/reconciliation gate
- `docs/N3_LIBRARY_INDEX_PLAN.md`
- `docs/baselines/N3_LIBRARY_INDEX_EVIDENCE.md`
- `docs/ARCHITECTURE.md`
- `docs/QUALITY_AND_TESTING.md`

