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

- Initial failed run: Windows CI `36280951746` (PR) and `36280936990` (push) exposed an Arabic FTS assertion using an inflected token (`قراءة`) while the fixture stored `القراءة`; `unicode61` does not stem Arabic. The inherited selected-route run `36280951747` failed at configure because that job did not use the new pinned SQLite toolchain.
- Fix commit `3baf78dfafbe77dc02d6c1b33c3b46a9f071caca` searches the exact stored Arabic token and adds the declared pinned vcpkg setup to selected-route CI.
- Verified on final commit `3baf78dfafbe77dc02d6c1b33c3b46a9f071caca`: Windows CI PR run `36281177576` passed Debug and Release; branch-push run `36281175089` passed Debug and Release; selected production route `36281177579` passed; coordinate `36281177581`, fidelity `36281177585` and stress `36281177658` checks passed. Thus N3.1 automated build, CTest, FTS5, Arabic/Urdu/English fixtures, migration rollback/version and inherited regression gates are green. Search currently requires matching indexed tokens; Arabic morphology/stemming is not claimed.

| Subgate | Status | Source commit | Verification/artifact | Known limits |
|---|---|---|---|---|
| N3.1 Storage/schema | **Complete** | `3baf78dfafbe77dc02d6c1b33c3b46a9f071caca` | Windows CI Debug/Release and all listed inherited PR gates passed on final head; run IDs above. | No local C++ toolchain; Windows CI is build/test evidence. Token matching only; no Arabic stemming claim. |
| N3.2 Scanner | In progress | Local implementation pending CI | Added streaming enumerator and path-only scanner; CTest evidence pending | Must verify on Windows; no index reconciliation, watcher, metadata extraction or owner test yet |
| N3.3 Identity/reconciliation | Not started | — | — | — |
| N3.4 Library UX/search | Not started | — | — | — |

The final N3 handoff must identify exact commit/workflow/artifact IDs, test results, artifact SHA-256 and explicit limitations. Physical owner test, mature-product comparison and owner `N3 PASS` must be recorded separately from CI.
