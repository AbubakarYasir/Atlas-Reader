# N4 Native Reader Foundation Evidence

<!-- atlas-status: N4|in-progress -->

## Opening record — 2026-09-28

- Accepted predecessor: N3 `2.0.0-beta.1`.
- Base merge: `739508e3e698016e8a6a4cb8b87c2b4aa418d66d` on `main`.
- Active branch: `native-v2-n4-reader-foundation`.
- Planned version: `2.0.0-beta.2`; the source remains `2.0.0-beta.1` until N4 earns promotion.
- Starting state: no N4 reader implementation or result is claimed.

## Subgate ledger

| Subgate | Status | Implementation SHA | Automated evidence | Rendered/owner evidence | Open blockers |
|---|---|---|---|---|---|
| N4.1 Session/open model | In progress | — | — | — | contracts and tests not yet implemented |
| N4.2 Virtual viewport | Not started | — | — | — | depends on accepted N4.1 contracts |
| N4.3 Navigation | Not started | — | — | — | depends on viewport geometry |
| N4.4 Outline/text/search/links | Not started | — | — | — | normalized reader adapters not implemented |
| N4.5 Workspace | Not started | — | — | — | production reader UI not implemented |

## Required final evidence

- Debug/Release test totals and exact GitHub run IDs at the proposed head.
- Package name, artifact ID/digest, inner ZIP SHA-256, build identity and clean-profile smoke.
- Source PDF before/after hashes proving N4 read-only behavior.
- Page geometry/rotation/link/search fixture results.
- 2,000-page and multi-tab memory/cache series with named hardware.
- Time-to-first-page, page-jump, scroll and search latency distribution.
- Rendered English/Arabic, light/dark/high-contrast, wide/narrow and 100%/200% review.
- Keyboard and Narrator/UIA route.
- Mature-reader workflow comparison and defect disposition.
- Owner checklist and explicit `N4 PASS`.

N4 is **In progress**, not Verified, Accepted or Released. This ledger must be updated with evidence as work lands; promises and passing older commits do not close a row.
