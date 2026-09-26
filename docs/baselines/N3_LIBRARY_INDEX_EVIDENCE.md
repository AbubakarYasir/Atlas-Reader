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

| Subgate | Status | Source commit | Verification/artifact | Known limits |
|---|---|---|---|---|
| N3.1 Storage/schema | In progress | Local source pending first CI run | Pending | Runtime/CI verification pending |
| N3.2 Scanner | Not started | — | — | — |
| N3.3 Identity/reconciliation | Not started | — | — | — |
| N3.4 Library UX/search | Not started | — | — | — |

The handoff must replace pending fields with exact commit/workflow/artifact IDs, test results, artifact SHA-256 and explicit limitations. Physical owner test, mature-product comparison and owner `N3 PASS` must be recorded separately from CI.
