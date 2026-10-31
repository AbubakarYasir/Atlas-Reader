# N4 Native Reader Foundation Evidence

<!-- atlas-status: N4|in-progress -->

## Process change — 2026-11-01

The owner chose **batched reviews** and a **focused 2.0** ([PLAN.md decision log](../../PLAN.md#9-decision-log)). Sub-stages now close on automated evidence plus a recorded rendered self-review; the owner reviews once at the end of N4 using [N4_REVIEW.md](../N4_REVIEW.md). N4.1 therefore closes as Implemented and automated-Verified, and **N4.2 is the active sub-stage**. The N4.1 rendered owner items (English/Arabic/RTL, 200%, keyboard/Narrator, restart/password, file release, source preservation) are carried into that review and still block `N4 PASS`. Side-by-side reading (C17) and the simplified text view (C18) left N4 scope and moved after 2.0. Sections below that mention a separate `N4.1 PASS` describe the earlier process and are kept as a historical record.

## Opening record — 2026-09-28

- Accepted predecessor: N3 `2.0.0-beta.1`.
- Base merge: `739508e3e698016e8a6a4cb8b87c2b4aa418d66d` on `main`.
- Active branch: `native-v2-n4-reader-foundation`.
- Planned version: `2.0.0-beta.2`; the source remains `2.0.0-beta.1` until N4 earns promotion.
- Starting state: no N4 reader implementation or result is claimed.

## Subgate ledger

| Subgate | Status | Implementation SHA | Automated evidence | Rendered/owner evidence | Open blockers |
|---|---|---|---|---|---|
| N4.1 Session/open model | **Implemented; automated Verified** | `5ac63df7da45d711380cfaac0e83f0b96deda7f4` | Windows CI run `36690884159`: Debug + Release build/test PASS, 43/43 tests; `atlas_reader_controller` PASS; inherited Fidelity `36690884055`, Stress `36690884080`, Coordinates `36690883965`, Selected Production Route `36690883948` PASS | owner checks folded into the end-of-N4 review ([guide](../N4_REVIEW.md), checks 1–4 and 9) | Narrator naming of the password field and Windows 200% scale remain unverified until that review |
| N4.2 Virtual viewport | **Active; not started** | — | — | — | — |
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

N4 is **In progress**, not Verified, Accepted or Released. PR #6 stays Draft until the owner records `N4 PASS`.

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

## N4.1 remaining owner gate (superseded on 2026-11-01; see Process change)

No automated evidence can replace the actual rendered Windows review. The owner must complete `docs/N4_1_OWNER_TEST.md`, covering the N4.1 surface in English/Arabic, RTL, 100%/200%, keyboard/Narrator, password/restart behavior, close-during-open behavior, file release and read-only/source-preservation observations.

N4.1 remains **Ready for owner test**, not Accepted, until defects from that review are fixed/retested and the owner records `N4.1 PASS`. N4.2 must not begin earlier.

## Review preparation and local setup repair — 2026-10-16

The active source checkout is now `native-v2-n4-reader-foundation`. The old `atlas_native_v2_n2` folder retained an incomplete dependency directory; the saved user `VCPKG_ROOT` still targeted it. Restored vcpkg under the active checkout's ignored `build/vcpkg`, checked out the exact declared baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`, bootstrapped it with metrics disabled and updated the saved user setting. Qt stays at 6.10.3; CMake/CTest resolve to the installed Visual Studio tools. `tools/dev/enter-windows-environment.ps1` now validates the kit, executable/toolchain files and baseline before setting the current process environment. No product dependency pin or accepted CI toolchain changed.

### Verification and corrections

- Clean local VS 2026 / MSVC 19.51.36246.0 / SDK 10.0.26100.0 configure and Debug/Release builds pass with warnings as errors.
- Local production-route CTest: **26/26 PASS in Debug and Release**. PDFium probes are disabled locally; the historical 43-test CI total includes the extra comparison probes and must not be substituted for these local totals.
- Extended the controller test to prove disabled-restoration restart, rejection of an HTTPS open request, filename-first presentation, separately retained document title and identical SHA-256 source bytes after ordinary/protected open, retry, restart, rename and cancellation/shutdown.
- Actual rendered review covered English light at 1260 × 800 and Arabic dark at 760 × 560 logical pixels. Additional English/Arabic dark screenshots use `QT_SCALE_FACTOR=2` at 760 × 560 logical pixels. Controls fit without clipping. This simulated Qt scale check does not replace the owner's Windows display-scale/Narrator review.
- Live Windows input verified Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+W and wrong/correct public fixture passwords submitted with Enter. UI Automation exposes named tab items, per-file close actions, Library/Open actions and restoration control; password text is masked.
- The inspection tool did not expose a password-field name or a reliable focused-element report. Password keyboard submission worked, but **Narrator naming/focus remains unverified** and must pass owner checklist 8 before N4.1 acceptance.
- Review corrected Reader primary names to filename stems while keeping embedded titles secondary, added keyboard/accessible tab selection and selected-tab visibility, kept close controls inset, removed the stale N3 UI label and replaced viewport implementation jargon with plain review-stage text.
- Raw screenshots and local test output stay under ignored `artifacts/n4-qa/`; no personal PDFs or profile data are committed.

### Delivery contract

`tools/release/package-n4-review.ps1` stages a separate N4.1 review ZIP with the dynamic Qt/PDF runtime, restored accepted font configuration, public fixture/provenance files, build/executable identity, notices, installed Qt SBOM documents and file/ZIP checksums. Its startup smoke uses a separate profile and removes Qt/plugin/import environment paths from the child environment. Existing output directories are preserved.

The N4 branch's Release CI now uploads the N4.1 review ZIP for 30 days. It no longer labels N4 builds as accepted N3 packages; the published N3 release remains immutable. The plain-language guide is `docs/N4_1_QUICK_START.md`. Package identity/checksum and current GitHub run results are recorded in the delivery follow-up after the review package is built. PR #6 stays Draft and unmerged, the source version remains beta.1 and owner `N4.1 PASS` is pending.
