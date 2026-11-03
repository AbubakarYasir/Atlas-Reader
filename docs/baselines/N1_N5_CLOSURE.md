# N1–N5 repair closure ledger

<!-- atlas-status: N5|in-progress -->

Started 2026-11-04. This ledger tracks the authorized repair pass, not acceptance.
Historical green tests do not qualify the repaired head. N5 remains unaccepted;
PR #8 stays Draft. No N6 work, release or merge is authorized by this pass.

| ID | Requirement and implementation | Required evidence | Current result |
|---|---|---|---|
| N1-01 | Windows startup and deployed Qt runtime | Clean-PATH package launch, no console or missing DLL | Pending |
| N1-02 | Settings, themes, three languages | Restart, narrow/wide and light/dark checks | Pending |
| N2-01 | Rendering and text correctness | Full nine PDF regression workflows on final head | Pending |
| N2-02 | Passwords, restrictions, signatures | Fixtures; no credentials in persisted data | Pending |
| N2-03 | Coordinates and preservation | Rotated/cropped destinations; independent reopen | Pending |
| N3-01 | Background scanning and cancellation | Existing scanner suite; incomplete scan never purges | Pending |
| N3-02 | Identity and offline roots | Moves/copies/ambiguity/offline regression checks | Pending |
| N3-03 | Filename-first metadata and library state | Unicode paths, Favorites/Recents, restart | Pending |
| N4-01 | Open/tabs/password/restore | Controller regression suite and keyboard checks | Pending |
| N4-02 | Virtual viewport and navigation | Long/mixed/rotated/link-heavy fixtures; memory bounds | Pending |
| N4-03 | Search and selection | Arabic/Urdu normalization; long-book p50/p95/p99 | Pending |
| N4-04 | Windows accessibility | Narrator, keyboard, high contrast, real scaling | Pending |
| N5-01 | Capability and overlay persistence | Full state/transaction/crash corpus | Pending |
| N5-02 | Editing, undo, auto headings | Tree/Unicode/10k-node tests and rendered review | Pending |
| N5-03 | Late-save failures | Reindex/commit failure restore and failed-restore tests | Local Windows regression passed; final CI pending |
| N5-04 | Durable recovery | Journal, process termination, restart, conflict preservation | Implemented; repair-head tests and Windows review in progress |
| N5-05 | Safe copy | Source alias and destination race checks | Hard-link/during-generation changes tested locally; broader audit pending |
| N5-06 | Reconcile external changes | Ambiguous matching and local-work preservation | Pending audit |
| N5-07 | Exchange responsiveness | Worker import/export, cancel, stale answers, large workload | Implemented; repair-head tests and Windows review in progress |
| N5-08 | Format fidelity | Unicode/limits/CSV/round-trip tests | Pending |
| UI-01 | Reference-based Windows review | EN/AR/UR, wide/narrow, 100/200%, empty/error/protected/conflict | Pending |
| PKG-01 | Correct review instructions | N5 START-HERE and guide in packaged ZIP | Script corrected; package pending |
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
and actual 200% display configuration; physical insufficient-space/removable-drive
and final-reopen failure qualification beyond injection; cryptographically signed
public fixture (A010 is a certification-structure marker, not a valid signature);
large reconciliation timing; full final-head CI, source/package traceability and
final clean-package smoke. Unknown-hash recovery currently blocks safely and retains
all evidence; an integrated explicit recovery-choice dialog is not yet implemented.
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
