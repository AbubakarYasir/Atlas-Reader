# N3 Library/Index Evidence Ledger

<!-- atlas-status: N3|in-progress -->

## Start record — 2026-09-27

- Predecessor N2: Accepted by owner; closure, branch cleanup and final checks recorded in [N2 acceptance](N2_ACCEPTANCE.md).
- N3 branch: `native-v2-n3-library-index-foundation`.
- Starting commit: `84a9b45b10b24ece595bb7069136fefd2ed2cc5b` on accepted native `main`.
- First active gate: N3.1 storage/schema.
- SQLite candidate pin chosen for N3.1: Microsoft vcpkg baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`, port `3.53.4#1`, explicit FTS5 feature, `x64-windows-static`. Runtime/build verification pending.
- Implementation at start: no SQLite package manifest, migration engine, Library repository, scanner or production Library UI in the accepted starting commit. The remaining `ILibraryIndex` type is only an interface.
- Owner N3 test and `N3 PASS`: pending; no feature or beta acceptance is implied by starting this branch.

## Subgate results

### First CI diagnostic — implementation `0912ab3486ac9427d73850069885d71497b6bda8`

- Windows CI Debug and Release configured and built the index adapter; all inherited PDF regression tests passed, but `atlas_library_index` failed because the Arabic query used an inflected token (`قراءة`) while the fixture stored `القراءة`. SQLite's `unicode61` tokenizer does not stem Arabic; the assertion now searches the exact stored token. This defines current token search behavior, not a claim of Arabic morphology support.
- The inherited N2 selected-production-route check failed at CMake configure because it did not use the newly active N3 SQLite manifest/toolchain. The workflow now bootstraps the same pinned vcpkg baseline and `x64-windows-static` toolchain as Windows CI.
- These are diagnosed fixes only; rerun results are pending. No N3.1 acceptance is claimed.

| Subgate | Status | Source commit | Verification/artifact | Known limits |
|---|---|---|---|---|
| N3.1 Storage/schema | In progress | `0912ab3486ac9427d73850069885d71497b6bda8` plus follow-up fix awaiting push | First Debug/Release run built; Arabic FTS expectation failed. Selected-route CMake lacked vcpkg. See first CI diagnostic above. | Fixes require new green CI; local C++ toolchain unavailable |
| N3.2 Scanner | Not started | — | — | — |
| N3.3 Identity/reconciliation | Not started | — | — | — |
| N3.4 Library UX/search | Not started | — | — | — |

The handoff must replace pending fields with exact commit/workflow/artifact IDs, test results, artifact SHA-256 and explicit limitations. Physical owner test, mature-product comparison and owner `N3 PASS` must be recorded separately from CI.
