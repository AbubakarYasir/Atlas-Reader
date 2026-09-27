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
| N3.2 Scanner | **Implemented; final exact-head CI pending** | `dd47f4a` | Traversal, overlap/deduplication, generation safety, inspection and coordination tests pass locally. Repeatable Release benchmark added. On the named owner SSD, 1,000 Unicode PDFs scanned in 1 ms and indexed in 541 ms. | Explicit Rescan, not an automatic watcher; slower/removable storage remains an owner test |
| N3.3 Identity/reconciliation | **Implemented; final exact-head CI pending** | `d14f1ea` | Local VS 2026 Debug/Release 24/24 each prove new stable documents, Unicode rename review/apply, stable ID, distinct copies, offline ambiguity and non-destructive replacement. Review actions are in the Library; all SQLite work is off the UI thread. | Packaged owner rename/copy/offline test remains |
| N3.4 Library UX/search | **Implemented; owner test pending** | `5e15fb2` | Real Library list/grid, folder filters, title/recent/relevance sorting, Favorites, Recents, safe phrase search, plain-language states and review actions. Rendered English wide, 760 px narrow, Arabic dark and 200% local checks passed after design review. | Narrator/removable-drive owner checks and mature-product comparison remain |

## Named local performance observation — 2026-09-27

- Source: Release benchmark target added at `dd47f4a`; final exact-head repetition is still required.
- Machine: HP Victus by HP Gaming Laptop 16-r1xxx; Intel Core i7-14650HX (16 cores/24 logical processors); 16 GB RAM.
- Storage: healthy `SKHynix_HFS001TDE9X081N` 1 TB NVMe SSD, NTFS system volume.
- Fixture: 1,000 copied 11,551-byte valid PDFs across ten nested folders with mixed Latin/Arabic filenames. The fixture and temporary benchmark database were removed after the run.
- Release result: enumeration `1 ms`; metadata/FTS indexing `541 ms`; 200 warmed search samples, p50 `647 us`, p95 `707 us`, 100-result cap.
- Interpretation: this is a reproducible storage/index smoke result, not a claim about every library or slower media. No removable/slower drive was connected; that required observation remains in the owner checklist.

## Declared N3 boundaries (not hidden loose ends)

| Boundary | Current behavior | Named owner and blocker |
|---|---|---|
| Continuous folder watching | N3 changes appear after explicit Add folder/Rescan (`F5`) | N7 Windows integration; event coalescing, bounded work and offline safety must pass before automatic watching ships |
| Arabic morphological search | N3 matches indexed words/phrases; it does not infer every inflected form | N4 reader search/N9 language qualification; missed promised query behavior blocks the owning checkpoint |
| Built-in PDF reading | N3 opens an available result in the current Windows PDF app | N4; Atlas Reader usability and correctness gates block N4 acceptance |
| OCR for image-only PDFs | N3 indexes available metadata only | Outside current N3 scope; any future OCR requires a separate documented checkpoint and privacy/performance contract |

## Final-CI corrective record

The first strict exact-head Debug job (`36289801631`, job `108537578568`) failed during QML cache compilation because MSVC promoted Qt 6.10.3's own `C4702` unreachable-code warning in Qt headers to an error. Atlas warnings remain errors. The correction suppresses only `C4702` on the QML executable target, where Qt-generated sources and Atlas `main.cpp` are compiled; all Atlas libraries and tests retain the full warning gate. A local Visual Studio 2026 Debug rebuild with `ATLAS_WARNINGS_AS_ERRORS=ON` passed before the correction was pushed for CI rerun.

The final N3 handoff must identify exact commit/workflow/artifact IDs, test results, artifact SHA-256 and explicit limitations. Physical owner test, mature-product comparison and owner `N3 PASS` must be recorded separately from CI using [the owner checklist](../N3_OWNER_TEST.md).
