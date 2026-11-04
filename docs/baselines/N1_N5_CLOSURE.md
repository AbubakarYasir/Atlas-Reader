# N1–N5 repair closure ledger

<!-- atlas-status: N5|in-progress -->

Started 2026-11-04. This ledger tracks the authorized repair pass, not acceptance.
Historical green tests do not qualify the repaired head. N5 remains unaccepted;
PR #8 stays Draft. No N6 work, release or merge is authorized by this pass.

| ID | Requirement and implementation | Required evidence | Current result |
|---|---|---|---|
| N1-01 | Windows startup and deployed Qt runtime | Clean-PATH package launch, no console or missing DLL | Local package launch passes; packaged qpdf runs; 1,519 file hashes verified |
| N1-02 | Settings, themes, three languages | `app_preferences_tests`, rendered matrix | Persistence suite passes; EN/AR/UR at real OS 175% and Night sky contrast; real 200% still open |
| N2-01 | Rendering and text correctness | Full nine PDF regression workflows on final head | Pending |
| N2-02 | Passwords, restrictions, signatures | Capability/security/save suites; A018 OpenSSL integrity + tamper | Local tests pass, including encrypted save and real signed-original refusal |
| N2-03 | Coordinates and preservation | Outline writer/save tests; nine PDF CI lanes | Local qpdf comparison + Qt PDF reopen pass; all-angle production regression passes; cloud CI unavailable |
| N3-01 | Background scanning and cancellation | `library_scanner_tests` and scan-service integration | Local Debug/Release passed; cloud CI unavailable (account billing) |
| N3-02 | Identity and offline roots | `document_identity_reconciler_tests`, `native_file_identity_tests` | Local moves/copies/ambiguity/offline checks passed |
| N3-03 | Filename-first metadata and library state | Library index/model/query/inspector suites | Local automated Unicode, metadata, Favorites/Recents and profile tests pass |
| N4-01 | Open/tabs/password/restore | Reader controller/session tests | Automated open/password/session tests pass; full keyboard review still open |
| N4-02 | Virtual viewport and navigation | Viewport layout/render suites, coordinate CI, Windows memory | Local mixed geometry/navigation pass; 2,000-page 400-jump memory measured; cloud CI unavailable (account billing) |
| N4-03 | Search and selection | Text-search suite; 2k-page viewport measurement below | Local tests and 100-query measurement passed |
| N4-04 | Windows accessibility | Narrator, keyboard, high contrast, real scaling | Actual contrast and 175% recovery screens verified; Narrator/native tree state and real 200% remain open |
| N5-01 | Capability and overlay persistence | Capability, overlay, store tests | Local state/transaction/crash corpus passes |
| N5-02 | Editing, undo, auto headings | Editor/model/auto-bookmark tests, V01 | Local Unicode/tree/undo/10k tests pass; Add Bookmark rendered and fixed |
| N5-03 | Late-save failures | Reindex/commit failure restore and failed-restore tests | Local Windows regression passed; cloud CI unavailable (account billing) |
| N5-04 | Durable recovery | Journal, six process-termination boundaries, explicit keep-current | Local regression passes; V04 real UI choice verified; cloud CI unavailable (account billing) |
| N5-05 | Safe copy | Source alias and destination race checks | Hard-link/during-generation changes tested locally; broader audit pending |
| N5-06 | Reconcile external changes | `reconcile_tests`, `outline_safe_save_tests`; 10k benchmark | Ambiguity regression passes; latest worker timing p95 382.476 ms |
| N5-07 | Exchange responsiveness | Worker import/export, cancel, stale answers, large workload | 10k workflow + cancellation/stale/one-undo tests pass locally; timings below |
| N5-08 | Format fidelity | `bookmark_exchange_tests`, `bookmark_model_tests` | Local Unicode/limits/CSV/round-trip and asynchronous undo tests pass |
| N5-09 | Malformed-outline safety (A016) | Opening A016 (outline `/Next` cycle) must not hang | Repaired: bounded raw-reference preflight rejects cycles before Qt traversal; A016 now runs in the normal safe-save regression |
| N5-10 | Recovery choice after an unsettled save | Keep current / restore original / show backups; nothing deleted | Integrated recovery dialog; verified archives before Keep/Restore, Show backups and Cancel; local Windows regression passes |
| UI-02 | Edit ribbon tab (Foxit placement of Bookmark, Auto Create Bookmarks) | Rendered EN/AR/UR | Edit ribbon rendered in EN/AR/UR; no extracted Foxit assets |
| UI-01 | Reference-based Windows review | EN/AR/UR, wide/narrow, 100/200%, empty/error/protected/conflict | Pending |
| PKG-01 | Correct review instructions | N5 START-HERE and guide in packaged ZIP | Local inspection ZIP built from 0d6d726; receipt below |
| GIT-01 | Owner identity and signatures | Audit every N5 commit; GitHub Verified | All N5 commits valid; corrected history pushed and verified |
| DOC-01 | Consistent contracts and status | Docs checker plus manual claim/evidence comparison | In progress |

## Preserved history and identity correction

The published history was preserved in a complete verified recovery bundle outside
the checkout before rewriting. The corrected local bundles were preserved unchanged.
The local signing public key matches the owner's GitHub signing key (ID 1291392).
All 17 earlier N5 commits returned `valid` from GitHub; the last two returned
`unknown_key` and incorrectly named Claude as author and committer.

| Published commit | Corrected signed commit |
|---|---|
| 5e55d07c32b2d34c93b0808598005ab109317067 | 2f676e374321a0ce86b306063e1dd61517400ba4 |
| 9a31534a29b592aef36a794fd7e57054919b6dba | cddac4a95577118b4217e09a6648125cba0f7ab2 |

Local signature verification passed and the corrected tip's tree equals the
published implementation tree exactly. Main and release tags are unchanged.
GitHub reports `valid` for both corrected commits and repair commit `40d36d5`.
The active local N5 branch and remote were equal after the authorized exact-lease push.

## Evidence boundaries

Historical run 41695365407 passed 59/59 Debug and Release at the old N5 head.
It is not evidence for this repair. Durable restart recovery, asynchronous exchange,
Windows visual/accessibility verification and the final package remain open.
Never summarize this ledger as "N5 complete" while any required row is open.

## First repair validation

Windows MSVC 19.51 / Qt 6.10.3 / pinned qpdf 12.4.1, warnings as errors:
Release 42/42 passed. Debug 42/42 passed, with the changed save target rechecked
after the last copy-safety test addition. Local configuration excludes 17 comparison-only
PDFium tests; this is not the 59-test CI result. Translation governance passes
475 keys in all three languages; Markdown governance passes 55 files.
The new save tests cover an actual SQLite commit exception, late reindex and commit
faults, failed restoration with backup retained, a source hard-link alias, and an
external destination edit during copy generation. This is not crash/restart proof.

## Second repair validation — in progress

Release MSVC 19.51, Qt 6.10.3, qpdf 12.4.1, Intel Core i7-14650HX (24 threads):
42/42 local CTest tests passed after recovery/exchange/reconcile repairs. Local
PDFium comparison probes remain disabled: the separate 59-test CI lane must pass
on the published repair head. Earlier head `56baf12` has all Windows and nine PDF
workflow checks green; that is not evidence for uncommitted changes.

- `outline_safe_save_tests`: child process exits at journal prepared, after replace,
  SQLite before commit and after overlay commit; fresh-store recovery checks hashes,
  revision and idempotence. An unknown external edit retains the journal and blocks save.
- `bookmark_model_tests`: 10,000 import read/preview 47 ms, apply plus database commit
  188 ms, export 67 ms; maximum measured UI heartbeat gap 17 ms on this run. Cancellation
  before each operation, single undo and stale completion after detach pass. These
  are single-run workflow measurements, not percentile claims or worst-case bounds.
- `atlas_n5_bookmark_benchmark`: 10k bookmark search p95 0.160 ms; editor command
  p95 0.213 ms; undo p95 0.310 ms. All existing benchmark gates pass.
- `reader_viewport_render_tests` with `ATLAS_LONG_BOOK=1`: generated 2,000-page
  searchable PDF; 100 warm queries through the real viewport/worker: p50 106.499 ms,
  p95 111.416 ms, p99 128.589 ms. Unique destinations and cancellation pass. Sparse
  synthetic pages are not a substitute for image-heavy or complex-layout books.
- Visual check V01 found the `currentRow` property/function collision: Ctrl+B did
  not create a bookmark. Removed the duplicate function; actual `bookmarks-add`
  QA screenshot shows the new local bookmark and its focused rename field.
- UI Automation exposes named document, controls and bookmark tree. Added the
  missing selectable flag and toast announcements. Narrator speech/expanded-state
  usability and real Windows contrast/scaling are still unverified.

### Explicit remaining checks

The owner requested a runnable inspection build. Providing it does not close these:
real Narrator workflow and native tree expanded/collapsed state; Windows contrast
and actual 200% display configuration; physical insufficient-space/removable-drive checks; full final-head CI, source/package traceability and
final clean-package smoke. Unknown-hash recovery now has an explicit keep-current choice, verified below.
Do not describe the inspection package as N5 complete while these remain open.

### Windows visual findings V02–V03

Twelve runtime captures covered EN/AR/UR with a wide 1280x850 light window at
Qt scale 1 and a narrow 760x500 dark window at Qt scale 2, both expanded bookmarks
and Export. These are process-level scale simulations, not a change to the Windows
monitor setting. The owner's runtime Foxit `screenshots/home-light.png` was inspected
for workspace hierarchy; no extracted resource was shipped.

V02: Arabic UI inherited a system fallback that rendered incomplete glyphs on this
machine. The interface now explicitly uses the existing bundled Noto Naskh Arabic.
V03: the Urdu Export popup exceeded a short scaled window. Export, Import and Save
now bound their height to the window, scroll the options and retain visible footer
actions. Final screenshots must be rechecked after rebuilding. The wording `Export…`
includes an intentional ellipsis because the next action opens a file picker.

### Rechecked repairs

After rebuilding, Arabic export text is complete and joined with bundled Naskh.
Urdu's scaled Export popup retains both action buttons; options scroll inside the
bounded dialog. These are actual Windows Qt screenshots, not reconstructed previews.
The final local Debug suite passes 42/42 (64.30 s), Release 42/42 (27.45 s).
A separate 20-run 10k reconciliation benchmark reports p50 143.365 ms and p95
161.199 ms on the worker. Final publication and clean-package checks remain pending
until recorded in PR #8; do not substitute earlier head results.

## Cloud audit — 2026-11-04 (in progress)

Head `cc12803`: all 11 workflows green (Windows CI Debug/Release, nine N2 PDF
workflows). Local Qt 6.4 Linux suite 25/25; qpdf 12.4.1 suites pass. The audit
compared every N5 promise (CHECKPOINTS N5, QUALITY.md N5 corpus and fuzzing,
DATA_AND_SAFETY, FOXIT_PARITY N5 rows, N4 items carried into N5) with code and tests.
Added in this pass:

- Edit ribbon tab (Hand, Select Text | Bookmark, Auto Create Bookmarks), as in Foxit's Edit → Links.
- Recovery choice for an unsettled save (N5-10) with tests (busy lock, restore keeps the
  replaced file among the backups, keep current, journal kept as `resolved-*.json`).
- Safe-save tests: too little free space (`space` fault) and, on Windows, a PDF held
  open by another program without write/delete sharing.
- Fuzzing of the bookmarks file and location envelopes (20,000 byte mutations,
  5,000 random trees, 50,000 envelope mutations); AddressSanitizer/UBSan clean.
- Fixtures A016 (malformed outline, readable pages) and A017 (real approval
  signature, pyHanko, throwaway certificate) with generators and manifest entries.
- Corpus sweep: A001–A006 and A015 take a 24-bookmark deep Arabic/English tree with
  duplicate names, save, reopen without reconcile; A017 is never written in place and
  Save a Copy works. A016 found defect N5-09 (above).

Still open from this audit: N5-09 fix; 10,000-bookmark panel navigation timing
(expand/collapse/reveal) in the benchmark; FOXIT_PARITY rows for security status and
add-bookmark-from-selection to update; rendered review of the Edit tab and recovery
buttons; Windows CI for these changes; the Windows-only rows above (Narrator, real
200% scaling, contrast, clean-package smoke) and the owner review.

A failed operating-system replacement is now hash-checked before clearing its
journal: if the destination differs from the original, the result is recovery
required and the backup/journal remain. A regression simulates a destination change
at this failure boundary. This avoids treating the OS error code as proof that
nothing changed.

## Third repair validation and remaining limits

- V04: actual Windows **Review recovery → Keep current file** exercised on a
  disposable, indexed synthetic book with an unknown external revision. The
  read-only hold was shown, the choice produced `before-save.pdf`, `kept-current.pdf`
  and `resolution.json`, the pending journal cleared, and the UI asked for reopening.
  Automated tests also preserve the hold after an interrupted resolution and check
  original/current hashes and unchanged overlay revision. Corrupt evidence remains
  held; the action does not restore over an external edit.
- A018 has a real detached RSA/SHA-256 CMS signature from an invented self-signed
  identity. Independent OpenSSL verification succeeds, altered signed bytes fail.
  Local safe-save tests refuse the signed original, preserve its hash and allow a
  working copy without claiming signature validity. A010 still covers certification
  structure; neither fixture is evidence of public certificate trust validation.
- `ATLAS_LONG_BOOK=1`: actual Windows private committed memory after each 50 jumps
  across 2,000 pages: **219, 292, 293, 294, 298, 307, 307, 309 MiB**. After 200 warm-up
  jumps the next 200 grew by 15 MiB, within the explicit 96 MiB regression ceiling.
  This measures this sparse PDF with software rendering, not every GPU/document.
  The subsequent 100 warm searches: p50 **108.622 ms**, p95 **129.761 ms**, p99
  **147.019 ms**. Hardware: i7-14650HX, Windows, Qt 6.10.3, Release.
- High contrast: the custom Theme now reads Qt 6.10's Windows contrast preference
  and system palette; borders remain visible. A process-only QA path can exercise
  the branch. A simulated capture is not an actual Windows contrast-theme pass.
- Review candidate increments to `2.0.0-beta.2-n5-review.2`; the earlier local
  `.1` archive is superseded, preserved for traceability, and not the final handoff.

Still **unverified**: full Narrator speech/focus flow and native expanded/collapsed
state exposure, real Windows 200% scaling (175% and Night sky now checked), actual storage-exhaustion
and removable-media failure. These remain engineering/owner checks, not accepted deferrals or passes.
The existing unit/API checks and synthetic fixtures cannot honestly close them.

A Windows-specific test now obtains a real sharing lock immediately after file
replacement, without returning an injected failure. The final Qt PDF reopen and
immediate restoration cannot read the file, so the journal must remain. Releasing
the lock allows restart recovery to restore the original hash. This covers the
actual locked-final-reopen path; it does not simulate a disconnected drive.

The full Release rerun exposed `atlas_reader_controller` failing its immediate
rename-after-shutdown check. Capability probes had discarded futures and could
outlive the controller. The controller now retains and joins these jobs at shutdown
and uses a monotonic request generation across reopening, preventing stale answers
from a previous capability-state generation. The earlier pass is superseded; this
repair requires targeted repeated lifecycle tests and both final full suites.

## Integrating owner-published cloud audit — 2026-11-04

Published `99a55af` was fast-forwarded into the existing Windows checkout. The
uncommitted Windows repairs were stashed with untracked files and preserved in a
verified complete recovery bundle before integration. No published commit, main,
release tag or legacy branch was rewritten. Removed the stale local fetch refspec
for the deleted N4 branch, which caused ordinary fetch to fail.

A016 remains the published malformed-outline fixture, A017 the cloud signed
fixture; the independently verified Windows signed fixture is A018. Both signature
fixtures are retained with separate provenance. Conflicting recovery controls are
combined into one Review recovery dialog with Keep, Restore, Show backups and Cancel.
Keep validates and archives both versions before clearing the hold; Restore keeps
the displaced file outside ordinary retention. Both require reopen before editing.

N5-09 repair under verification: raw outline object links are checked through the
existing qpdf process boundary before constructing Qt's eager bookmark model. Cycles,
repeated references, excessive depth/node count or failed structural inspection
leave the outline unsupported and read-only; page rendering remains available.
The formerly opt-in A016 regression now runs in the ordinary safe-save suite.
This changes no PDF engine or source PDF. Worker inspection has bounded output and
a timeout. The temporary inspection copy retains the source bytes and encryption.

The model integration test now measures 30 expand/collapse/reveal samples against
10,005 bookmarks with nested children. These measure model work, not rendered frame
latency. Measured model timings are recorded below; final local Debug/Release results are recorded separately. The security
status and selection-to-bookmark Foxit comparison rows now describe the implemented
behavior and distinguish signature presence from certificate trust validation.

This integration is not yet qualified or published. Do not use the old green
cc12803 CI results to claim that the combined build passes.

### Local-only verification decision and additional N4 finding

GitHub jobs on `99a55af` never started: job annotations report an account billing
or spending-limit refusal. The owner requested local verification instead. This
is recorded as unavailable online CI, not a code/test failure or a green check.
The pinned comparison PDFium archive was checksum-verified and enabled locally;
it is not part of Atlas's production PDF engine or review distribution.

The raw coordinate probe passed its historical limitation-aware criteria, but
revealed a production gap: Qt's link destination omits the target page's inherent
rotation. Atlas now uses qpdf raw page rotation metadata to normalize both link
and outline destinations before navigation/overlay adoption. A012 is retained
byte-for-byte with its existing provenance; production regression covers 0/90/180/270-degree links plus save/reopen on a rotated destination. This closes an N4
carry-forward defect rather than accepting the engine limitation as product behavior.

Local Release passed 59/59 before this additional rotation repair; Debug passed
42/42 before enabling comparison probes. Both final configurations must be rerun
after the rotation repair. Six qpdf structure/security/write workflows and the
coordinate, raster-fidelity and stress validators passed locally. Fifteen corpus
entries pass independent pypdf 6.12.2 validation; both A017 and A018 pass OpenSSL
CMS integrity and tamper rejection. No certificate-trust claim is made.

Visual captures reviewed the Edit ribbon and recovery dialog in EN/AR/UR, light/
dark, wide/narrow and process-level 200% scaling. Recovery Escape cancels safely.
Initial action focus was missing; Cancel now receives focus on opening. Real
Windows scale/contrast and Narrator speech remain unverified: attempts to inspect
Windows settings encountered intervening user input; system settings were not changed.

Release panel model timings after avoiding redundant document-wide recounts: expand
p50 6.412 / p95 19.593 / p99 20.319 ms; collapse p95 0.0655 ms; reveal
p95 0.2264 ms. Thirty samples per operation, 10,005 nested bookmarks. These
are not rendered-frame or screen-reader timings. Nested 10k import read 63 ms,
transactional preparation/commit 2,301 ms, export 87 ms, UI heartbeat maximum
23 ms. Debug uses a longer functional-test timeout, not a performance exemption.

### Combined Windows source validation

The integrated code passed **59/59 Release (47.37 s)** and **59/59 Debug
(138.72 s)** on Windows / MSVC 19.51 / Qt 6.10.3 with warnings as errors and
the pinned PDFium comparison probes enabled. The final small QML focus-reason
change is rebuilt and visually checked separately. Translation governance passes
494 keys in all three languages; Markdown governance passes 55 files.
The independent fixture validator passes all 15 registered entries.

Artifacts remain local under `artifacts/n5-final-*-tests.log`,
`artifacts/n5-local-pdf-matrix/`, `artifacts/n5-local-qpdf-regressions/` and
`artifacts/n5-integrated-qa/`. These are repair-build evidence, not N5 acceptance.

Final integrated long-book rerun: 2,000 pages, 400 navigation jumps, private
committed memory **210, 294, 297, 304, 306, 306, 309, 309 MiB**; the last
200 jumps grew by 5 MiB (96 MiB regression ceiling). One hundred sequential
warm searches: **p50 306.452 / p95 493.163 / p99 531.691 ms**. This is
slower than the earlier run and is retained as measured, not replaced with the
better historical result. Search correctness/cancellation passed; QUALITY.md
requires these percentiles to be recorded, not a new invented timing threshold.

### Owner-authorized real Windows display check

Original Windows settings: 100% scale, 1920x1080, contrast theme None. The
standard scale menu exposes 100/125/150/175%, not 200%. Recovery dialogs were
rendered and inspected in EN/AR/UR at actual OS 175% (application telemetry
confirms device-pixel ratio 1.750), including all four actions and visible Cancel
focus. All fit at 850x560 logical pixels. Real 200% is still unverified; no
custom-scale sign-out was performed.

Windows Night sky contrast was applied, then EN/AR/UR recovery dialogs were
inspected at 100%: system colors, borders and focus remained visible. These
are actual OS contrast captures, distinct from the process-only QA switch.
Artifacts: `artifacts/n5-integrated-qa/*-os175-recovery.png` and
`*-os-contrast-recovery.png`. Both original settings were restored and verified
in Windows Settings. This does not claim a complete Narrator workflow pass.

Latest 10k bookmark benchmark: search p95 0.770 ms (limit 100), editor command
p95 0.369 ms and undo p95 0.663 ms (limit 2). Reconciliation worker p95
382.476 ms is recorded, not substituted by the search result. All defined
benchmark gates pass. The four Qt/PDFium A003/A006 benchmark probes also pass
and identify version `2.0.0-beta.2-n5-review.2`.

### Local inspection delivery receipt — 2026-11-04

Implementation: `0d6d7264f0441edd2025fa39e0245cf7b4cc1c71`, owner-authored,
SSH signed, GitHub Verified. Exact-head local suites: Release **59/59 in
50.09 s**, Debug **59/59 in 139.87 s**. Cloud CI remains unavailable.

Portable version: `2.0.0-beta.2-n5-review.2`. Local folder, relative to the
parent of the checkout: `review-builds/N5-repair-0d6d726/`. Archive:
`atlas-reader-review-windows-x64.zip`, 51,126,748 bytes. SHA-256:
`630d6910bfe196fe8f1680e4dd778246d0dac3b8e253edf680848631a71dbb89`.
All 1,519 entries in SHA256SUMS verified. Clean-PATH GUI launch and standalone
packaged qpdf 12.4.1 passed. Includes N5 guide, public samples, provenance,
licenses and exact executable/source identity. Package was not uploaded.
The archived guide/ledger reflects implementation-commit evidence; this receipt
is a later documentation-only update, not different executable bytes.

**New measured open defect N1-03:** startup misses QUALITY.md targets. After
other tests finished, three warm clean-PATH launches took 5,306.381 / 4,885.479 /
5,249.321 ms to first frame. The first run spent 4,687.606 ms loading QML.
Do not describe startup as qualified or blame concurrent tests: the isolated
recheck reproduced it. Package launch correctness is separate from launch speed.

Outstanding engineering/owner gates: startup profiling and repair; native tree
accessibility and Narrator speech/focus workflows; complete keyboard/state matrix;
actual 200% scale; rendered 10k expand/jump latency; actual storage exhaustion
and removable-media failure; requirement-by-requirement final closure and N5 PASS.
This receipt does not label N5 complete or authorize merge/release/N6.
