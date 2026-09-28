# N3 Library/Index Acceptance

**Checkpoint:** N3 — Library/index foundation

**Version:** `2.0.0-beta.1`

**Accepted:** 2026-09-27

**Owner decision:** `N3 PASS`

## Accepted result

N3 is Accepted. Atlas now has a useful native Windows Library foundation: explicit folders, bounded background PDF indexing, filename-first English/Arabic/Urdu search, Favorites, Recents, list/grid views, durable identity across safe moves and honest offline/unavailable states.

The owner exercised checks 1–11, reported visible and metadata defects, retested the corrections, and then issued the exact acceptance words `N3 PASS`. The required mature-library comparison was completed separately with Calibre 9.9. The detailed implementation, test, package, checksum, competitor-comparison and corrective evidence remains in [the N3 evidence ledger](N3_LIBRARY_INDEX_EVIDENCE.md).

## Exact accepted branch evidence

- Final pre-acceptance branch head: `ed12d162c26aa86d35918a295b94c624599a870b`.
- Windows push run `36314538436`: Debug and Release passed.
- Windows pull-request run `36314541482`: Debug and Release passed.
- Coordinate run `36314541601`, fidelity run `36314541626`, stress run `36314541433`, and selected-route run `36314541503` passed.
- GitHub artifact ID `10930131644`, name `atlas-reader-n3-windows-x64-ed12d162c26aa86d35918a295b94c624599a870b`, workflow digest `sha256:f38ec762419099a1302dd27889b37dc9dda077f12b5ebd5cbaad91a7115191d5`.
- The preceding exact owner package at implementation-equivalent head `1db167faf9a3a16b097bed898cb3c431af4f18fd` was downloaded, checksum-verified and clean-profile smoke-tested; later changes through `ed12d16` were documentation-only.

The acceptance/version synchronization commit and final `main` merge subsequently passed their exact-head CI. PR #5 merged as `739508e3e698016e8a6a4cb8b87c2b4aa418d66d`; all eight permanent-branch workflows passed and the completed N3 branch was deleted locally/remotely.

## Release record — 2026-09-28

- Immutable tag: `v2.0.0-beta.1`, pointing to verified merge `739508e3e698016e8a6a4cb8b87c2b4aa418d66d`.
- GitHub prerelease: `https://github.com/AbubakarYasir/Atlas-Reader/releases/tag/v2.0.0-beta.1`.
- Windows x64 ZIP SHA-256: `75b2ca930c60175e9e8dd97048fba62a131efc47b5bb26691e48b1004fe87692`, verified against the packaged checksum before publication.
- The release is an accepted development beta, not stable 2.0 and not a claim that the built-in Reader exists.

## Frozen boundaries

- N3 discovers and opens PDF files only. EPUB, comics and user-owned DRM-free Kindle-family formats remain planned later checkpoints.
- N3 opens books in the configured Windows PDF application. The built-in Atlas reader begins in N4.
- Grid cover/first-page previews, bounded file watching and richer metadata editing remain assigned to later checkpoints.
- OCR, audio notebooks, online services and collaboration are outside Windows 2.0 scope.
- Review is required only for identity-sensitive moves, renames and replacements; ordinary new/unchanged files do not require per-file approval.

## Next gate

N4 may start only from the verified accepted `main` merge. N4 owns the native PDF reader foundation; it must not reopen N3 unless new evidence proves a regression.
