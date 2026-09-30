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
| N4.1 Session/open model | In progress | pending checkpoint commit | Debug build; focused `atlas_reader_session`, `atlas_library_document_inspector` and smoke tests pass locally | UI launches cleanly; full rendered/owner review not yet performed | restoration/resource-release component tests, password-entry flow, Windows sharing proof and full QA remain |
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

## N4.1 partial implementation checkpoint — 2026-09-30

Implemented, without claiming N4.1 completion:

- an engine-independent read-session model with explicit loading, ready, password-required, missing, malformed, unsupported-security, cancelled and failed states;
- duplicate-path activation instead of duplicate tabs, deterministic activation/close behavior, reopen revisions and rejection of stale or post-close asynchronous results;
- background Qt PDF inspection for direct local files and Library Open actions, returning only Atlas-owned metadata to the interface;
- an initial tab strip and honest in-app ready/error surface; no page-rendering claim is made before N4.2;
- optional local tab restoration, disabled by default, storing paths only and never passwords;
- direct PDF opening through `Ctrl+Shift+O`, tab close through `Ctrl+W`, and a route back to the accepted Library.

Focused local evidence on Visual Studio 2026/Qt 6.10.3 Debug:

- `atlas_reader_session`: PASS;
- `atlas_library_document_inspector`: PASS;
- `atlas_core_smoke`: PASS;
- `atlas_reader.exe --quit-after-ms 1200` with an isolated profile: clean exit.

This is a resumable source checkpoint, not an owner package. N4.1 remains **In progress** because restart/restoration, clean release, cancellation/resource release, Windows file-sharing, password entry, full Debug/Release regression, rendered accessibility/RTL review and GitHub CI evidence have not all been completed.
