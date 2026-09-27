# N3 Library/Index Evidence Ledger

<!-- atlas-status: N3|ready-for-owner-test -->

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
| N3.2 Scanner | **Ready for owner test** | `dd47f4a` | Traversal, overlap/deduplication, generation safety, inspection and coordination tests pass locally and on final exact-head CI. Repeatable Release benchmark added. On the named owner SSD, 1,000 Unicode PDFs scanned in 1 ms and indexed in 541 ms. | Explicit Rescan, not an automatic watcher; slower/removable storage remains an owner test |
| N3.3 Identity/reconciliation | **Ready for owner test** | `d14f1ea` | Local VS 2026 Debug/Release 24/24 each plus final exact-head CI prove new stable documents, Unicode rename review/apply, stable ID, distinct copies, offline ambiguity and non-destructive replacement. Review actions are in the Library; all SQLite work is off the UI thread. | Packaged owner rename/copy/offline test remains |
| N3.4 Library UX/search | **Ready for owner test** | `5e15fb2` | Real Library list/grid, folder filters, title/recent/relevance sorting, Favorites, Recents, safe phrase search, plain-language states and review actions. Rendered English wide, 760 px narrow, Arabic dark and 200% local checks passed after design review. | Narrator/removable-drive owner checks and mature-product comparison remain |

## Named local performance observation — 2026-09-27

- Source: Release benchmark target added at `dd47f4a`; repeated against final implementation head `acfe8df8f18ba59500d7a786c93bdf6df96cb16d` before the documentation-only handoff commit.
- Machine: HP Victus by HP Gaming Laptop 16-r1xxx; Intel Core i7-14650HX (16 cores/24 logical processors); 16 GB RAM.
- Storage: healthy `SKHynix_HFS001TDE9X081N` 1 TB NVMe SSD, NTFS system volume.
- Fixture: 1,000 copied 11,551-byte valid PDFs across ten nested folders with mixed Latin/Arabic filenames. The fixture and temporary benchmark database were removed after the run.
- Release result: enumeration `1 ms`; metadata/FTS indexing `541 ms`; 200 warmed search samples, p50 `647 us`, p95 `707 us`, 100-result cap.
- Final implementation-head repetition: enumeration `1 ms`; metadata/FTS indexing `542 ms`; 200 warmed search samples, p50 `626 us`, p95 `695 us`, 100-result cap. The copied fixture and temporary database were removed after the successful run.
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

## Ready-for-owner-test handoff — 2026-09-27

- Final implementation/package head: `2c84f39ba1935702c14bb3fe841de86422bcd6ba`; it includes the owner-selected standardized Windows icon and the serialized PDFium probe deployment correction.
- Windows CI branch run `36291263574`: Debug job `108541678730` and Release job `108541678616` passed. PR run `36291260247`: Debug job `108541669463` and Release job `108541669601` passed. Both Release builds completed in parallel without the prior shared-DLL race.
- Inherited regression runs on the same implementation head passed: selected route `36291263467`, coordinates `36291263451`, fidelity `36291263446`, stress `36291263508`.
- GitHub artifact ID `10921709418`, name `atlas-reader-n3-windows-x64-2c84f39ba1935702c14bb3fe841de86422bcd6ba`, workflow-artifact digest `sha256:7710120eb7cc682b3c4fb92bcd934d6b31ba8d518f57bb801ee907fe30ecbbb6`.
- Owner ZIP: `atlas-reader-n3-windows-x64.zip`, 40,156,396 bytes, SHA-256 `95ecd6491c501a850f4ed0a6620d809e0bd80c8178cf05c0232682b50ea44f0c`. The downloaded checksum file and a fresh local `Get-FileHash` result match.
- The staged package contains the Release executable, required Qt runtime files, plain-language owner checklist and build identity. CI launched the staged executable without the earlier missing-DLL failure. The downloaded packaged executable's actual 32 px Windows icon was extracted and visually confirmed against the supplied Atlas mark.

All planned implementation and automated gates are complete, so N3 is **Ready for owner test**, not Accepted. The physical removable/offline-root check, Narrator/keyboard review, mature-product comparison and explicit owner `N3 PASS` must still be recorded using [the owner checklist](../N3_OWNER_TEST.md). N4 remains closed.

### Documentation-head CI reliability follow-up

Documentation-head run `36290792972` exposed an inherited nondeterministic Release-build race rather than a test or product failure: parallel PDFium qualification targets attempted to deploy the same `pdfium.dll` into one output directory, and one copy received `permission denied`. Commit `398fe81` explicitly orders the Windows probe targets for their shared post-link deployment while unrelated compilation remains parallel. Final head `2c84f39ba1935702c14bb3fe841de86422bcd6ba` passes both Debug/Release runs and every inherited workflow listed above, so this reliability follow-up is closed.

## Owner-test round 1 and corrective verification — 2026-09-27

The owner reported checks 1–11 as working overall and skipped check 12, while identifying three release-quality defects: a terminal remained behind the app, some PDF cards displayed stale embedded values such as `1.docx`, raw hexadecimal metadata or an old `.inp` path instead of the current filename, and the Open action visually touched the card boundary. The owner also asked why file-change review appears and noted that Grid has no first-page preview.

- Atlas now builds `atlas_reader.exe` as a Windows GUI-subsystem application. The corrected Release binary reports PE subsystem `2` (Windows GUI), so a console is not created for ordinary launches.
- A centralized metadata policy rejects implausible embedded titles/authors and uses the current Unicode PDF filename for display. It handles source-document names, raw PDF hex strings, embedded local paths, control/replacement characters and oversized values. Existing indexed rows benefit immediately at display time; newly inspected PDFs store the corrected title.
- The List/Grid action row now has an explicit height, bottom inset and bounded action widths. Actual rendered English wide, 760 px narrow and Arabic/RTL dark screenshots show the Open action fully inside the card with consistent surrounding space.
- The review dialog and owner checklist now state the exact safety rule: new and unchanged files need no review; only identity-sensitive moves, renames or replacements appear, and each proposed identity transfer requires an individual owner decision.
- Grid cover/first-page thumbnails are not missing N3 work. N3 intentionally uses a PDF placeholder; cached visual previews are owned by N7 and must meet its cache, invalidation and performance gates before shipping.
- Local Visual Studio 2026 Debug and Release builds pass all 24 tests in each configuration, including new bad-metadata policy cases and all inherited PDF/Arabic/Urdu checks.

Exact corrected implementation/package head `7a3a217710c7bcba41ce1ce240d21f6adb9aab1f` passed Windows push run `36292988234` and PR run `36292990517` in Debug and Release. The same head passed selected-route run `36292990473`, coordinate run `36292990489`, fidelity run `36292990532` and stress run `36292990635`.

GitHub artifact `10922991795`, `atlas-reader-n3-windows-x64-7a3a217710c7bcba41ce1ce240d21f6adb9aab1f`, is 79,598,673 bytes with workflow digest `sha256:d29cd01ac91430ab6a205f899260360041c04e9b3ed2d0ea4737bc654db47ab0`; the downloaded outer artifact matches that digest exactly. Its owner ZIP is 40,165,221 bytes with SHA-256 `70ac892127ed838b7d93ff78cfe3c10ab3479567867a13363e0f4c86f0d279e8`, matching the packaged checksum file. The packaged `N3_BUILD_INFO.txt` identifies the same implementation/checkout SHA and Release configuration; the packaged executable independently reports Windows GUI subsystem `2`.

This closes the identified implementation defects but does not manufacture owner acceptance. Owner retest of the corrected package is required; skipped owner check 12 also remains open. N3 therefore remains **Ready for owner test**, and N4 remains closed.
