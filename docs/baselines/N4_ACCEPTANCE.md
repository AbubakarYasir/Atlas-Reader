# N4 Reader Acceptance

**Checkpoint:** N4 — Reader

**Version:** `2.0.0-beta.2`

**Accepted:** 2026-11-02

**Owner decision:** `N4 PASS`

## Accepted result

N4 is Accepted. Atlas reads PDFs in its own Foxit-style workspace in English, Arabic and Urdu: tabs and passwords, a bounded virtual viewport, page display modes and Reverse View, Pages and Bookmarks panels, Find and the Search panel with Arabic/Urdu normalization, text selection, SnapShot, Word Count, links with confirmation, per-book view memory and the Preferences pages that go with them. DerekSol is named as publisher in the program and About.

The owner wrote `N4 PASS` on 2026-11-02, after the N4 build was merged into `main` on the owner's direction. Detailed implementation and test evidence: [N4 evidence ledger](N4_READER_FOUNDATION_EVIDENCE.md).

## Exact accepted evidence

- Final stage head `8fef3a9dd6d5bb8f098e5dc474f02ee1d1419d42` (signed, Verified): Windows push run `41400925190` and pull-request run `41400931412` (Debug and Release, 46/46), N2 coordinate, fidelity, stress and selected-route runs passed.
- PR #6 merged as `45c395b97c8f875fbbc0ef42591026496c331588` (Verified); the stage branch was deleted on GitHub and locally.
- Owner review build: exact-head package, checksum verified on the owner's PC (`review-builds/N4-final-8fef3a9`).

## Open items carried forward (not blocking, by owner decision)

Accepted with these known gaps, each owned by a later stage:

- Not yet measured or covered by fixtures: search p50/p95/p99 on a long book, link-heavy and rotated-destination fixtures, Narrator and 200% scale checks, side-by-side Foxit screenshots of the new panels → N5 automated QA (measurements) and N10 (hardware and accessibility pass).
- Moved features: Open a Copy, image context menu, Destinations panel, single-key accelerators, Preferences → Full Screen/Reading → N7; search in comments → N6; tiling beyond 8,192 px, tagged reading order, one shared in-memory PDF copy → N10.
- "Unknown publisher" until a DerekSol code-signing certificate signs the build → N11 (earlier if the certificate is ready).

## Frozen boundaries

- N4 is read-only: no PDF is ever written. Bookmark editing, annotations and saving start in N5/N6.
