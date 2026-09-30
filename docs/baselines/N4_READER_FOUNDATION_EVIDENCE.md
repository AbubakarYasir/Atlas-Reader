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
| N4.1 Session/open model | **Ready for owner test** | `5ac63df7da45d711380cfaac0e83f0b96deda7f4` | Windows CI run `36690884159`: Debug + Release build/test PASS, 43/43 tests; `atlas_reader_controller` PASS; inherited Fidelity `36690884055`, Stress `36690884080`, Coordinates `36690883965`, Selected Production Route `36690883948` PASS | owner checklist in `docs/N4_1_OWNER_TEST.md` pending | rendered English/Arabic/RTL, 100%/200%, keyboard/Narrator and owner runtime verdict |
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

N4 is **In progress**, not Verified, Accepted or Released. N4.1 being Ready for owner test does not accept N4 and does not authorize merging PR #6.

## N4.1 implementation checkpoint — 2026-09-30

Implemented without beginning N4.2:

- an engine-independent read-session model with explicit loading, ready, password-required, missing, malformed, unsupported-security, cancelled and failed states;
- duplicate-path activation instead of duplicate tabs, deterministic activation/close behavior, reopen revisions and rejection of stale or post-close asynchronous results;
- background Qt PDF inspection for direct local files and Library Open actions, returning only Atlas-owned metadata to the interface;
- an initial tab strip and honest in-app ready/error surface; no page-rendering claim is made before N4.2;
- optional local tab restoration, disabled by default, storing paths only and never passwords;
- direct PDF opening through `Ctrl+Shift+O`, tab close through `Ctrl+W`, and a route back to the accepted Library;
- password submission through an obscured field, specific wrong-password retry state, successful unlock through Qt PDF, and field clearing after submission;
- password non-persistence: restart restores only source paths and a protected document returns to password-required state;
- cancellation ownership: closing/retrying retires the superseded worker, prevents stale results from mutating the session, keeps the worker owned until completion, and waits for active/retired workers during controller shutdown;
- revision/watcher-specific cleanup so an old completion cannot erase a newer retry watcher;
- corrected Reader-body session actions so retry/close target the actual session delegate rather than an intermediate layout object.

### Automated Windows evidence

Implementation head: `5ac63df7da45d711380cfaac0e83f0b96deda7f4`.

Windows CI run `36690884159` on Windows Server 2022 / MSVC 2022 / Qt 6.10.3:

- Debug Configure: PASS;
- Debug Build: PASS;
- Debug CTest: **43/43 PASS**;
- Release Configure: PASS;
- Release Build: PASS;
- Release CTest: **PASS**;
- Release artifact/evidence staging: PASS.

The new `atlas_reader_controller` integration test passes and covers:

- ordinary asynchronous open to ready state and page count;
- ready-session Windows rename/move proof, demonstrating the inspection path does not retain an unnecessary file handle;
- encrypted open → password-required;
- wrong password → explicit retry state;
- correct password → ready;
- correct and rejected passwords absent from the persisted `reader-session.ini`;
- opt-in path-only session restoration across a fresh controller instance;
- protected restored session asks for the password again;
- repeated close-during-open cancellation;
- controller shutdown waits for retired workers and the PDF can be renamed after shutdown, proving no orphan worker retains the Windows file handle.

Inherited exact-head regressions also pass:

- N2 PDF Fidelity run `36690884055`;
- N2 PDF Stress run `36690884080`;
- N2 PDF Coordinates run `36690883965`;
- N2 Selected Production Route run `36690883948`.

The earlier local Visual Studio 2026/Qt 6.10.3 Debug evidence remains useful historical evidence but is superseded for N4.1 closure by the exact implementation-head GitHub Debug/Release evidence above.

## N4.1 remaining owner gate

No automated evidence can replace the actual rendered Windows review. The owner must complete `docs/N4_1_OWNER_TEST.md`, covering the N4.1 surface in English/Arabic, RTL, 100%/200%, keyboard/Narrator, password/restart behavior, close-during-open behavior, file release and read-only/source-preservation observations.

N4.1 remains **Ready for owner test**, not Accepted, until defects from that review are fixed/retested and the owner records `N4.1 PASS`. N4.2 must not begin earlier.
