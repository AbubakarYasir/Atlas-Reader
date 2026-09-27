# N3 Library/Index Evidence Ledger

<!-- atlas-status: N3|in-progress -->

## Start record — 2026-09-27

- Predecessor N2: Accepted by owner; closure, branch cleanup and final checks recorded in [N2 acceptance](N2_ACCEPTANCE.md).
- N3 branch: `native-v2-n3-library-index-foundation`.
- Starting commit: `84a9b45b10b24ece595bb7069136fefd2ed2cc5b` on accepted native `main`.
- First active gate: N3.1 storage/schema.
- At N3 start, SQLite candidate pin initially used `x64-windows-static`; a later runtime audit detected that this triplet's static CRT conflicts with the Qt dynamic CRT. The active triplet is corrected to `x64-windows-static-md`. This start-state note is superseded by the completed verification record below.
- Implementation at start: no SQLite package manifest, migration engine, Library repository, scanner or production Library UI in the accepted starting commit. The remaining `ILibraryIndex` type is only an interface.
- Owner N3 test and `N3 PASS`: pending; no feature or beta acceptance is implied by starting this branch.

## Subgate results

### First CI diagnostic — implementation `0912ab3486ac9427d73850069885d71497b6bda8`

- Initial failed run: Windows CI `36280951746` (PR) and `36280936990` (push) exposed an Arabic FTS assertion using an inflected token (`قراءة`) while the fixture stored `القراءة`; `unicode61` does not stem Arabic. The inherited selected-route run `36280951747` failed at configure because that job did not use the new pinned SQLite toolchain.
- Fix commit `3baf78dfafbe77dc02d6c1b33c3b46a9f071caca` searches the exact stored Arabic token and adds the declared pinned vcpkg setup to selected-route CI.
- Verified on final commit `3baf78dfafbe77dc02d6c1b33c3b46a9f071caca`: Windows CI PR run `36281177576` passed Debug and Release; branch-push run `36281175089` passed Debug and Release; selected production route `36281177579` passed; coordinate `36281177581`, fidelity `36281177585` and stress `36281177658` checks passed. Thus N3.1 automated build, CTest, FTS5, Arabic/Urdu/English fixtures, migration rollback/version and inherited regression gates are green. Search currently requires matching indexed tokens; Arabic morphology/stemming is not claimed.
- Runtime audit follow-up: run `36281662994` (and local VS 2026 build) emitted MSVC `LNK4098` because the old `x64-windows-static` triplet used static CRT while the Qt build uses dynamic CRT. Changed CMake presets and both CI configure paths to pinned-baseline triplet `x64-windows-static-md` (static SQLite library, dynamic CRT). Local Visual Studio 2026 Debug and Release builds each passed 20/20 CTest checks without LNK4098. Final head `e7fe595c5ffb228f48b63c17a960559a8b8b8a22` also passed Windows PR run `36283212258` Debug/Release, branch run `36283210492` Debug/Release, selected route `36283212263`, coordinates `36283212368`, fidelity `36283212328`, and stress `36283212278`. The final CI logs contain no LNK4098 warning.

| Subgate | Status | Source commit | Verification/artifact | Known limits |
|---|---|---|---|---|
| N3.1 Storage/schema | **Complete** | `e7fe595c5ffb228f48b63c17a960559a8b8b8a22` | Local VS 2026 Debug/Release 20/20 each; Windows PR run `36283212258` and branch run `36283210492` Debug/Release; selected route `36283212263`; no LNK4098 on corrected triplet. | Matching FTS tokens only; Arabic stemming is not claimed |
| N3.2 Scanner | In progress | `30137a1` | Traversal, overlap/deduplication, generation safety and scan/index coordination passed local/Windows through `4da8f5c`. PDF inspection of readable, Unicode, malformed, locked and missing fixtures passes local VS 2026 Debug and Release 21/21 each at `30137a1`; final branch CI pending. | New paths intentionally remain unlinked pending identity/reconciliation; no watcher, named-storage measurement or owner test yet |
| N3.3 Identity/reconciliation | In progress | `0ee0237` | ADR-0005 policy; SQLite schema v4 reviewable proposal/apply; native Windows file ID; scanner observation integration. Local VS 2026 Debug/Release 23/23 each prove restart persistence, no pre-apply mutation, stable Atlas ID across a proven move, offline ambiguity rejection, same native ID after rename, distinct ID after copy, and a different object at a known path published as a replacement without reassignment/deletion. | Candidate-to-proposal orchestration, final CI and owner test remain |
| N3.4 Library UX/search | Not started | — | — | — |

The final N3 handoff must identify exact commit/workflow/artifact IDs, test results, artifact SHA-256 and explicit limitations. Physical owner test, mature-product comparison and owner `N3 PASS` must be recorded separately from CI.
