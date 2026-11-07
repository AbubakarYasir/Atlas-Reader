# Changelog

<!-- atlas-status: N5|in-progress -->

All notable changes to Atlas Reader Native are documented here.

## N1–N5 repair pass — 2026-11-04 (in progress)

- 2026-11-08 — Narrator: bookmark rows report open/closed natively (Expand/Collapse work), focus follows the current bookmark or book instead of staying on the list, and dialogs are announced by name ([ADR-0010](docs/decisions/ADR-0010-screen-reader-interfaces.md)). Focus starts on the first book and moves into the File page.
- 2026-11-08 — Windows CI now presses real keys through UI Automation (EN/AR/UR), renders the main screens at 100 %, Windows 125 %, 200 % and in high contrast, and smoke-tests the review package from a clean PATH.
- 2026-11-08 — Auto Create leaves headings that are already bookmarks unchecked; in Urdu its list of headings is visible again. Counts read correctly for any number ("Results: 1"). Arabic Zoom menu renamed so it differs from Zoom in.
- 2026-11-08 — Every menu uses one density, measured against Foxit Reader's bookmark menu (23 px rows; Atlas 25 px in English, growing for Arabic and Urdu).

- 2026-11-07 — Startup: bundled fonts are registered once before the interface loads instead of by QML FontLoaders, which made Qt rebuild its list of every installed font. Windows CI first frame is now 470–767 ms (EN/AR/UR, target 800 ms); it was 4.9–5.3 s on the owner PC.
- 2026-11-07 — Saving: a full backup drive no longer reports an interrupted save; a drive removed mid-save leaves a recovery that settles once the original is back. New RAM-disk tests for full and unplugged drives, and a broader Save a Copy audit.
- 2026-11-07 — Accessibility: bookmark rows are tree items (level, open/closed, page, state) instead of buttons; radio buttons, check boxes and menu commands are exposed. A new accessibility dump and checker runs on Windows CI in EN/AR/UR across ten surfaces.
- 2026-11-07 — Arabic and Urdu wording review; Urdu sidebar rows no longer overlap.
- 2026-11-07 — The repository is public so GitHub-hosted CI runs again. All nine N2 workflows and Windows CI pass.

- Local inspection package delivered from 0d6d726; both final 59-test Windows suites pass. Independent warm launches expose a 4.9–5.3 s startup target miss, which remains an open repair. Actual OS 175% and Night sky recovery screens verified in all three languages; real 200% and Narrator are not counted as passed.

- Preflight raw outline references before Qt constructs its bookmark model; cyclic A016 now runs in the normal regression suite without exhausting memory. Unsupported outlines remain protected.
- Normalize link and outline destinations for inherent page rotation, with production tests at all four right-angle rotations and a rotated bookmark save/reopen.
- Consolidate recovery choices into one dialog, validate evidence before resolving, preserve displaced versions outside ordinary retention, and focus Cancel initially.
- Measure nested 10,000-bookmark import/export and panel model operations; avoid recomputing document-wide counts during expand/collapse. Measurements distinguish model work from rendered frames.
- Integrate the three published audit commits without rewriting history; perform Windows verification locally at the owner's request while GitHub Actions is blocked by account billing.

- Reader shutdown now waits for background capability probes; globally increasing request IDs prevent an old probe from updating a reopened tab. The full-suite cancellation test exposed the lifetime bug.
- Custom theme colours now honor Windows high-contrast preferences through the system palette.
- Explicit **Review recovery → Keep current file** preserves verified copies of both versions and a resolution receipt before clearing a recovery hold; local research is unchanged and the reader must reopen. Invalid backups or another concurrent edit leave the hold in place.
- Added a cryptographically signed synthetic PDF, independent OpenSSL integrity/tamper checks, signed-original refusal and working-copy regression. No production signing feature or trusted-identity claim is added.
- Added actual Windows private-memory sampling during 400 jumps in a 2,000-page document. The last 200 jumps grew from 294 to 309 MiB; this sparse-fixture check does not qualify every complex book.


- Added a versioned recovery journal, per-book save lock and six actual process-termination recovery tests; unknown external revisions are preserved and block further saves.
- Import/export and import merge preparation now run on the existing worker, with busy/cancel state and stale-result rejection. Tested a 10,000-bookmark import/export, single-step undo and tab closure.
- Ambiguous outline identities no longer match by order. Fixed the bookmark panel's duplicate `currentRow` property/function name; added selectable accessibility state and spoken toast announcements.
- Arabic UI now explicitly uses the bundled Naskh font. Save/Import/Export dialogs keep actions visible with scrollable content in short windows, including Urdu.
- Added an opt-in 2,000-page end-to-end search measurement. Review binaries use `beta.2-n5-review.2`, not the accepted beta.2 or a released beta.3.

- Late save failures now attempt verified atomic restoration without deleting the current PDF first. A failed restoration has an explicit recovery-required message in all three languages.
- Added regression checks for final reopen, overlay commit and failed restoration, including a real database transaction exception.
- N5 packaging selects the N5 review instructions, includes the closure ledger and a bookmark exchange sample, and checks that the package version matches the executable. START-HERE links to the packaged guide so its relative evidence links work. Added a requirement closure ledger; restart recovery and worker exchange are implemented, with their final qualification tracked explicitly.
- Corrected the last two N5 commits locally to owner authorship and registered SSH signing; added a pre-push identity/signature gate.
- This entry supersedes the earlier claim that N5 was complete: acceptance waits for the closure ledger and owner review.

## N5.5: export and import bookmarks — 2026-11-03

- **Export Bookmarks…** (Bookmarks panel ⋯, or the title-bar search): save your bookmarks as an Atlas bookmarks file (complete, with notes and tags, and can be imported again), as Markdown for reading, or as CSV for a spreadsheet. Export all of them, one bookmark with everything under it, or only what you added or changed in Atlas.
- **Import Bookmarks…**: Atlas checks the file first and shows what it holds (and warns if it came from another book). Add the bookmarks at the end, under the selected bookmark, at the end without the ones already here, or instead of the current ones. One Undo takes the whole import out; the PDF changes only when you save.
- Files from a newer Atlas Reader are refused with a clear message, and a damaged file imports nothing.
- The file format is published (`docs/formats/atlas-bookmarks.md`).
- Historical readiness claim withdrawn by the 2026-11-04 repair audit; see the closure ledger.

## N5.4 (part 3): bookmarks changed elsewhere, and Save a Copy — 2026-11-03

- If another program changed a PDF's bookmarks, Atlas now catches up when you open the book: new bookmarks appear, changed ones update, removed ones go, and the panel tells you what happened. Your own bookmarks and edits are kept. If a bookmark was changed both in Atlas and in the PDF, it is marked; right-click it to keep your version or use the PDF's. If most of the PDF's bookmarks seem to have vanished, Atlas asks before removing them.
- **Save a Copy with Bookmarks** (Ctrl+Shift+S): for books Atlas can't change in place (read-only, in use, signed, certified), save a new PDF with your bookmarks in it. The original stays untouched.
- Review builds are now made only when asked for.

## N5.4 (part 2): Save bookmarks into the PDF — 2026-11-03

- Press **Ctrl+S** (or the Save button at the top left) to put the bookmarks you see into the PDF itself, so other PDF readers show them too. The first time for a book, Atlas says how many will be added and changed, and offers to also remove the PDF's own bookmarks you deleted in Atlas (they are kept unless you choose this).
- Saving is careful: Atlas writes a copy, checks it, compares it with the original, opens it again, keeps a backup of the original in your profile, and only then replaces the PDF. Failures before replacement preserve the original. Later failures attempt validated restoration; unresolved recovery retains the files and blocks another save.
- Protected, signed, certified, read-only or in-use PDFs are never changed; your bookmarks stay in Atlas. A PDF changed by another program since Atlas read it is never overwritten.
- After saving, the bookmarks show as being in the PDF; notes and tags stay in Atlas. Undo starts fresh after a save.
- Auto Create Bookmarks now keeps headings of the same level side by side when a level is missing.

## N5.4 (part 1): groundwork for saving bookmarks into the PDF — 2026-11-03

- Behind the coming Save: Atlas can now write a bookmark tree into a copy of a PDF and prove it is right before anything is replaced: the copy is checked by qpdf, every untouched bookmark keeps everything it had, nothing outside the bookmarks changes (pages, links, forms, document details, encryption), and Atlas's own PDF engine reads the result back. Encrypted PDFs stay encrypted with the same passwords. Nothing in the app writes PDFs yet.
- Decided: Save will put the bookmarks you see into the PDF. A PDF bookmark you deleted in Atlas stays in the PDF unless you choose to remove it when saving; notes and tags stay in Atlas.

## N5.3 (part 4): Auto Create Bookmarks — 2026-11-03

- In the Bookmarks panel's options (or the Command Center), choose **Auto Create Bookmarks…**. Atlas reads the whole book in the background and suggests headings: lines in larger type, and short lines that start like a heading in English, Arabic or Urdu (Chapter, الفصل, باب …). Running headers, footers and page numbers are left out, and a heading over two lines becomes one bookmark.
- Check the preview, untick anything you don't want, choose how many levels (1–4) and whether the new bookmarks go after, before or instead of the existing ones, then create them. One Undo takes them all back. Nothing is written into the PDF.
- Atlas estimates type size from what the PDF engine reports, so bold headings at body size are found only when they start like a heading; the preview is there so you decide.
- Radio buttons in Preferences and Skins now match the check boxes in every skin.
- With this, the bookmark editor (N5.3) is complete; next comes saving bookmarks into the PDF safely (N5.4).

## N5.3 (part 3): Command Center, Undo/Redo, Identity, Security, Signatures — 2026-11-03

- **Command Center:** click the magnifier in the title bar or press **Alt+Q**, then type. Atlas lists matching commands (with their shortcuts), books in your library and bookmarks in the open book; use ↑/↓ and Enter, or Esc to close. Commands can be found by their English name in any interface language.
- **Undo and Redo** buttons in the Quick Access Toolbar, and Ctrl+Z / Ctrl+Y anywhere in the reader, undo and redo your bookmark edits. Save comes with safe saving in the next part.
- **Preferences → Identity:** your name, organization and email, for notes and markups later. **Preferences → Security:** whether Atlas re-checks a file when you return to its tab, and whether the status bar shows the protection label. Preferences categories are now in alphabetical order, as in Foxit.
- **Signatures panel** (left rail): which signature fields a PDF has, whether each is signed, on which page, and whether the document is certified. Atlas does not check whether signatures are valid, and says so.
- Fixes: books outside your library get an **Add This Book's Folder** button in the Bookmarks panel; the bookmark marked as current now follows the top of the view, not just the page; Home → Bookmark uses the plain bookmark icon; disabled title-bar buttons are dimmed correctly in the colourful skin.
- Three icons added from the same pinned Fluent UI System Icons commit (signature, undo, redo).

## N5.3 (part 2): an editable Bookmarks panel — 2026-11-03

- Press **Ctrl+B** (or Home → Bookmark, or right-click the page) to bookmark what you are reading; name it straight away, or keep the selected text or page number Atlas suggests.
- In the Bookmarks panel: rename with F2, move with Alt+↑/↓, nest under the bookmark above with Alt+→ (Alt+← in Arabic and Urdu) and out again with the opposite arrow, set a bookmark to the current view, undo and redo, and find any bookmark by its title, notes or tags.
- Bookmarks that came with the PDF are never removed from the book: deleting one hides it in Atlas, and "N bookmarks are hidden" brings them back. Small icons say which bookmarks live only in Atlas or were changed in Atlas; hover or use a screen reader for the words.
- Books outside your library show their own bookmarks read-only, with a note on how to add the folder so you can keep your own.
- Five icons added from the same pinned Fluent UI System Icons commit (bookmark add, delete, edit, eye, eye off) for these commands.

## N5.3 (part 1): the bookmark editing engine — 2026-11-03

- Behind the coming Bookmarks panel: add, rename, move, nest and reorder bookmarks, give them notes and tags, and undo or redo any of it. Deleting a bookmark that belongs to the PDF hides it in Atlas instead, so the book itself is never changed by accident; change a PDF bookmark back and Atlas knows it matches the book again.
- Bookmark search covers titles, notes and tags, and finds Arabic and Urdu words with or without harakat.
- Fast on big books: with 10,000 bookmarks, searching takes well under a millisecond and every edit or undo under 2 ms; saving happens in the background. Windows checks these limits automatically on every build.

## N5.2: bookmarks kept safely in Atlas — 2026-11-03

- Groundwork for the bookmark editor: Atlas can now keep your bookmarks in its own profile, next to the book's own table of contents, and remembers for each one whether it is in the PDF, only in Atlas, waiting to be saved into the PDF, changed in Atlas, hidden in Atlas, or in conflict. Nothing is ever shown as saved into the PDF until it really is.
- Bookmark titles, notes and tags are kept exactly as typed in any language. Bookmarks point at a precise place on a page, not just a page number, and survive a crash or power cut.
- You will see this in N5.3, when the Bookmarks panel becomes editable. The profile is upgraded automatically (schema 5).

## N5.1: what Atlas may change — 2026-11-02

- Atlas now checks every PDF you open: password-protected, restricted by its owner, signed or certified, read-only, in use by another program, or changed outside Atlas. When Atlas may not change a book, a small label in the status bar says so; click it to see why in plain words. Your bookmarks and notes stay in Atlas either way.
- The Windows build includes qpdf 12.4.1 (Apache-2.0) for these checks.

## 2.0.0-beta.2: Reader accepted — 2026-11-02

- Owner wrote **N4 PASS**. Atlas Reader `2.0.0-beta.2` is the first beta that reads PDFs inside Atlas ([acceptance](docs/baselines/N4_ACCEPTANCE.md)).
- `main` now builds the accepted beta package; stage branches build an owner review package.
- N5 (bookmarks and document safety) has started.

## DerekSol as publisher; N4 merged to main — 2026-11-02

- Atlas Reader is published by **DerekSol** ([dereksol.com](https://dereksol.com)). The Windows program now carries DerekSol as company, copyright and description (Properties → Details); About shows DerekSol and how to reach them for custom software; Help has a **Custom Software** button.
- Windows will still say "Unknown publisher" until the program is signed with a code-signing certificate issued to DerekSol; how and when is in [WORKFLOW.md](docs/WORKFLOW.md#5-versions-and-releases) (planned with the installer, N11).
- Windows build: sources are compiled as UTF-8, and printed page labels fall back to page numbers on Qt 6.10.
- N4 is merged into `main` on the owner's direction; the end-of-N4 review continues there.

## N4.4 and N4.5 complete: find, search, bookmarks, links — 2026-11-02

- **Find and Search:** Ctrl+F opens a Find bar over the page (Enter or F3 for the next match, Shift+F3 for the previous one); Ctrl+Shift+F opens the Search panel on the right with every result and the words around it. Arabic and Urdu search ignores harakat, tatweel and letter shapes, so العلم finds العِلْمُ.
- **Bookmarks panel:** the book's own table of contents in the left strip; the current chapter is highlighted.
- **Select Text, SnapShot and Copy:** select text with Alt+6 and copy with Ctrl+C; SnapShot (Alt+7) copies any area as a picture; Word Count on the View tab.
- **Links:** links inside the book jump there (Alt+Left comes back); web links show their address and open only when you confirm.
- **More zoom:** Fit Visible (Ctrl+3), Zoom To (Ctrl+M), Marquee Zoom and Foxit's full zoom list; AutoScroll (Ctrl+Shift+H).
- **Atlas remembers each book:** page, zoom and layout come back when you reopen it. New Preferences pages: Documents, History, Page Display, Search.
- **Recent files:** pin, copy path, open the folder, remove; pinned files stay at the top.
- Printed page numbers ("iv", "xii") work in the page box. Books without searchable text say so plainly.
- Screen readers now hear the text of the current page.
- Icons added from the same pinned Fluent UI commit for the new commands: cursor, screenshot, copy, clipboard, text_word_count, open, arrow_up, arrow_down, arrow_autofit_down, search_square.

## N4.4 navigation, slices 1–2 — 2026-11-02

- **Page display like Foxit:** Single Page, Continuous, Facing (two pages side by side), Continuous Facing, and Show Cover Page — from the View tab or the status bar.
- **Reverse View** for Arabic and Urdu books: two-page spreads read right to left, and the Left arrow turns forward.

- **Pages panel:** page thumbnails in a side panel opened from the new left icon strip (or F4); click a page to jump; drag the edge to resize, double-click it to hide. Arabic and Urdu mirror it to the right.
- **Previous View / Next View** (Alt+Left / Alt+Right, and two new status-bar buttons) take you back to where you were before a jump.
- **Right-click a document tab:** Close, Close Others, Close Tabs to the Left/Right, Close All, Copy Path, Open Containing Folder.
- Commits are now signed with the owner's key and show Verified on GitHub; earlier commits were re-signed.

## Complete Foxit feature plan and logo colours — 2026-11-01

- **Every Foxit PDF Editor feature is now planned** (owner direction). [FOXIT_PARITY.md §3](docs/FOXIT_PARITY.md#3-complete-foxit-feature-catalogue-and-atlas-plan) catalogues all of them from the owner's reference package — 11 ribbon tabs, the classic menu bar, 114 menus, 159 core dialogs, 34 Preferences categories, every navigation panel and context menu, 107 modules and 62 plug-ins — and gives each a stage before 2.0 or an after-2.0 track.
- [AFTER_2_0.md](docs/AFTER_2_0.md) is reorganized into tracks **A–K** (Reading Plus, Formats, Pages/Create/Convert, Edit, Forms, Protect & Sign, OCR/Accessibility/Standards, Automate, Review & Share, Connected opt-in, Platforms). Offline by default stays; connected features are opt-in. Only cracking, bypasses and hidden data collection are never built.
- Pre-2.0 stages gained the Foxit items they own (N4.4 tab/page/recent menus and rail; N4.5 search options, Word Count, link/image menus; N5 Auto Create Bookmarks, Signatures panel; N6 full Comment tab and panel; N7 Help tab, menu bar, tray; N9 Reading Option).
- **Atlas's main colour now comes from its logo:** Atlas Blue accent (`#0b4fae`, Sky `#6cc4ff` in dark mode), navy title bar (`#0b3d91`), cyan highlight; theme colours Atlas Blue, Midnight, Sky, Teal, Plum, Graphite. Green is gone.

## N4.3 Foxit workspace — 2026-11-01

- **Atlas now looks and works like Foxit PDF Editor** (owner decision, [ADR-0007](docs/decisions/ADR-0007-foxit-workspace-parity.md)): Atlas's own 48 px title bar with Quick Access Toolbar (Open, Mouse/Touch mode) and window buttons; ribbon tabs **File · Home · View · Accessibility · Help** with Foxit-sized buttons and menus (Hand, Zoom, Page Fit Option, Rotate View, Read Mode, Full Screen, Visual Mode, View Setting, Reading Option, Touch Mode, About); collapsible ribbon (Ctrl+F11); Foxit-style document tabs with the **Library as the first tab**; Foxit-style status bar (« ‹ 12 / 340 › », zoom slider and percentage menu, full screen).
- **File page** like Foxit's: Open (Recent Documents, Computer), Close, Close All, Skins, Preferences, Exit. Recent documents are remembered.
- **Preferences window** (Ctrl+K) with a searchable category list: General, Languages, Skins, Accessibility; Cancel undoes changes.
- **Skins:** Light / Dark / Use system setting, and Classic or Colourful title bar with six Atlas theme colours.
- Foxit shortcuts: Ctrl+K Preferences, Ctrl+Q exit, Ctrl+F11 ribbon, Ctrl+Shift+Page Up/Down tabs; Add folder moves to Ctrl+Shift+O; Ctrl+F searches the Library.
- Fixed: text on green buttons and the title bar was drawn black because two colour names started with "on" (QML reads those as signal handlers).
- New study [docs/FOXIT_PARITY.md](docs/FOXIT_PARITY.md): measured layout and every Foxit command mapped to before or after 2.0. Plans updated everywhere.

## N4.3 workspace revamp and three languages — 2026-11-01

- **Atlas speaks English, Arabic and Urdu.** Pick the language from the title bar; the whole interface switches instantly, mirrors for Arabic and Urdu, and is remembered. Urdu uses the bundled Noto Nastaliq Urdu font. Book titles, names and paths in any script display correctly in every language.
- **New workspace design** (Foxit-style layout, Atlas look): title bar, document tabs, a command bar, and a status bar with page navigation, fit buttons and a zoom slider. New icons throughout.
- **Read mode** (Ctrl+H), **Full screen** (F11), view **rotation**, **reading colours** (Paper, Sepia, Night, High contrast — the PDF itself never changes), **Light / Dark / System** appearance and **Touch mode**.
- Library restyled with list and grid views.
- PDFs passed on the command line open in the reader.
- New CI check: every interface string must exist in all three languages.

## N4.2 page display — 2026-11-01

- **Atlas now shows PDF pages.** Opening a book shows a smooth, virtualized page view fitted to the window width, with a page box, page count, zoom out/in, Fit width and Fit page, scroll bars, wheel and keyboard navigation, and Ctrl+wheel zoom at the pointer. Only visible and nearby pages render, on a background thread; texture memory is capped.
- Open PDFs are read into memory once, so the file is never locked and never changed.
- *Restore reader tabs* moved into the reader's "More" menu.
- New test `atlas_reader_viewport_render` renders a generated 60-page PDF offscreen and checks display, clipping, page jumps, zoom, fit page, file release and unchanged bytes.

## N4.2 core viewport model — 2026-11-01

- Added `atlas_reader_viewport`: engine-independent page layout, render planning with scale buckets, a cancelling render scheduler and a byte-budgeted texture cache with pinning. New `atlas_reader_viewport` test includes a 2,000-page memory series (peak 236 MiB within a 256 MiB budget). No UI change yet; page display arrives with the Qt render adapter.

## Focused 2.0 plan and documentation consolidation — 2026-11-01

- **Scope (owner decision):** Windows 2.0 now means Library + Reader + Bookmarks + annotations + everyday tools + backup/migration + Arabic/accessibility qualification. EPUB, comics, DRM-free Kindle files, the KFX decision, PDF editing tools (E1–E5), form filling (C20), side-by-side reading (C17), the reflow view (C18) and read-aloud (C11) moved to [after 2.0](docs/AFTER_2_0.md), with their requirements kept.
- **Process (owner decision):** one owner review per stage instead of per sub-stage. Sub-stages close on automated evidence plus a recorded rendered self-review. N4.1 closes as Implemented and automated-Verified; its owner checks move into the end-of-N4 [review guide](docs/N4_REVIEW.md); **N4.2 is now active**. The next stage may start once a review build is delivered; merge, tag and release still need the owner's `N# PASS`.
- **Versions renumbered** after beta.1 (nothing later was published): N5 beta.3, N6 beta.4, N7 beta.5, N8 beta.6, N9 beta.7, N10 rc.1, N11 2.0.0.
- **Docs consolidated:** 35 reference files in `docs/` became 9 plus an index (DESIGN, ARCHITECTURE, DATA_AND_SAFETY, QUALITY, WORKFLOW, BUILDING, DEPENDENCIES, AFTER_2_0, N4_REVIEW). `INFO.md` was merged into README and CHECKPOINTS. README, PLAN, CHECKPOINTS, AGENTS and CONTRIBUTING were rewritten. Every previous file is preserved unchanged under [docs/history/plan-v1](docs/history/README.md).
- **New design system** ([DESIGN.md](docs/DESIGN.md)): character, measured contrast for the existing palette, a new `controlBorder` token for WCAG 1.4.11, type scale with Arabic rules, spacing, radii, motion, components, keyboard map, a design-review matrix, and a plan for a `Theme` singleton plus Qt Linguist translations in N4.5.
- **Tooling:** the docs governance check now validates the new document set, skips frozen history, and requires Automated QA / Owner review / Stop gate sections for N4–N11. CI copies of the N2/N3 plans point at their history location. The review packager ships `docs/N4_REVIEW.md` as `START-HERE.md`.
- No product code, dependency pin, toolchain or source version changed.

## N4.1 review-build preparation — 2026-10-16

- Synchronized the public roadmap, checkpoint summary, current notes and agent/documentation entry points with the N4.1 Ready for owner review gate. The earlier partial implementation entry below remains historical.
- Added a validated Windows environment helper for the active checkout, replacing reliance on the removed old checkout's vcpkg path while retaining the exact accepted dependency baseline.
- Added separate N4.1 review packaging with runtime deployment, build identity, fixture provenance, notices, file/archive checksums and a packaged startup check. N4 branch builds must no longer be labelled as accepted N3 packages.
- Added plain-language review instructions. N4.1 owner acceptance, N4.2 page display, beta.2 promotion and merge remain pending; no product version or dependency pin changes.
- Rendered/source review found that Reader tabs used embedded titles and lacked keyboard selection. Reader primary names now use filename stems with document titles secondary, tab semantics include name/selected state, Enter/Space and Ctrl+Tab/Ctrl+Shift+Tab work, selected tabs stay visible, and close controls remain inset. Replaced the viewport implementation jargon with plain review-stage wording.
- Extended controller integration checks for disabled-restoration restart, rejected network URLs and unchanged SHA-256 source bytes after the complete open/password/restoration/cancellation lifecycle.

## N4 start — 2026-09-28

- Opened `native-v2-n4-reader-foundation` from verified accepted `main` merge `739508e3e698016e8a6a4cb8b87c2b4aa418d66d` only after PR #5 merged, all permanent-branch workflows passed and the completed N3 branch was deleted locally/remotely.
- Set N4 to **In progress** with N4.1 session/open model as the only active subgate. The source stays `2.0.0-beta.1`; `2.0.0-beta.2` is earned only at N4 acceptance.
- Added a binding read-only reader plan and empty evidence ledger covering virtualized rendering, mixed geometry/rotation, zoom/fit/navigation, outlines/text/search/links, tabs/restoration, side-by-side and simplified text view, offline security, Arabic/Urdu, accessibility, rendered UI review, bounded memory and exact owner/package gates.
- N5 bookmarks, N6 ink/annotations and every PDF mutation remain closed. N4 must prove source hashes unchanged.

## N4.1 partial checkpoint — 2026-09-30

- Added the first Atlas-owned read-session contract and focused lifecycle tests: asynchronous open states, duplicate-tab policy, deterministic tab activation/close, reopen revisions and stale-result rejection.
- Routed Library and direct local PDF opens into an initial in-app tab workspace instead of the external Windows reader. The workspace currently proves open/session/error state only; virtual page rendering remains N4.2 and is not claimed.
- Added opt-in local tab restoration, disabled by default and storing no passwords, plus clear missing, malformed, password-required and unsupported-security states.
- Local Visual Studio 2026/Qt 6.10.3 Debug build, complete 25/25 CTest suite and isolated-profile startup pass. N4.1 remains In progress pending its remaining component, sharing, Release, rendered accessibility/RTL and CI evidence.

## `2.0.0-beta.1` released — 2026-09-28

- Published the first accepted native Library beta as immutable tag `v2.0.0-beta.1` from verified N3 merge `739508e3e698016e8a6a4cb8b87c2b4aa418d66d`.
- Attached the exact Windows x64 ZIP and checksum file to the GitHub prerelease. The ZIP SHA-256 is `75b2ca930c60175e9e8dd97048fba62a131efc47b5bb26691e48b1004fe87692`.
- The beta remains an early evaluation build: it provides the accepted Library foundation and opens PDFs externally; the in-app Reader starts in N4.

## N3 owner acceptance — 2026-09-27

- Owner recorded exact `N3 PASS` after exercising checks 1–11, reviewing the filename-first corrections and receiving the completed Calibre 9.9 comparison for check 12.
- Promoted the native engineering version from `2.0.0-alpha.2` to `2.0.0-beta.1`, the planned first useful Library beta milestone.
- Added the frozen [N3 acceptance record](docs/baselines/N3_ACCEPTANCE.md) and synchronized live status documentation. The separate release act was completed on 2026-09-28 as recorded above.
- N4 may start only from the verified accepted `main` merge on `native-v2-n4-reader-foundation`; the completed N3 branch is deleted after ancestry and remote synchronization checks.

## Planning corrections — 2026-09-27

- Owner confirmed expanded offline 2.0 scope: complete annotation controls, E1 page organization, E2 original PDF text/image editing and equations, E3 forms, E4 secure redaction and E5 later-stage offline signatures. OCR, audio notebooks and online/collaboration remain excluded. Each stage has dependency qualification, actionable work, tests and owner gates; no implementation claim or version bump.
- N8/N9 planned release slots move to beta.14/beta.15 to accommodate E1–E5; N3 was subsequently accepted on 2026-09-27.

- Clarified the owner's competitor audit is about missing 2.0 capabilities and quality, not copying Foxit's design. Recorded supplied Xournal++ and separate Foxit Reader/Editor references, evidence limits, required annotation visibility and honest unsupported forms/signature behavior, plus explicit dispositions for notebook/audio/LaTeX/plugins, OCR, redaction, AI and suite features.
- Added concrete automated and owner tests for F1–F4; planned features remain unimplemented. N3 acceptance does not claim those later formats.

- Owner moved ebook/comic delivery before Windows 2.0: N7 → F1 EPUB → F2 CBZ/CBR → F3 user-owned, DRM-free AZW3, MOBI and PRC files → F4 KFX decision → E1–E5 offline editing → N8 backup/migration → N9 accessibility → RC/stable. Final planned N8/N9 slots are beta.14/beta.15.
- Added stage-owned competitive workflow requirements and tests for focused reading/history, document accessibility, active-content containment, durable ebook locations, pen controls, annotation lists, sorting/filtering, safe rename, offline read-aloud and mixed-format recovery.
- Synchronized release strategy, scope, documentation index and public roadmap. Added explicit open-source dependency obligations. N3 remains PDF-only and awaits owner acceptance.

## [Unreleased]

### N3 start — 2026-09-27

- Began N3 after N2 owner acceptance, merge, final CI, local/remote synchronization and cleanup were verified. Created `native-v2-n3-library-index-foundation` from accepted native `main` commit `84a9b45b10b24ece595bb7069136fefd2ed2cc5b`.
- Set N3 to In progress and N3.1 storage/schema as its first subgate. Added the binding N3 plan for database/repository/search, scanning, identity/reconciliation, Library UX, QA, owner review and beta stop gate, plus an evidence ledger with no tests/results claimed at start.
- At the initial N3 start record, product code and dependency pins had not yet been established. The engineering version remains alpha.2 until the N3 acceptance/first-useful-beta gates are met.
- Chosen and declared SQLite `3.53.4#1`/FTS5 on pinned vcpkg baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`; added a private SQLite-backed repository adapter with versioned transactional schema migrations, root/book/favorite/recents persistence, FTS triggers/rebuild and isolated CTest coverage. Windows Debug/Release builds and tests pass on CI. Existing root IDs and book locations cannot be reassigned to different paths/documents without an explicit reconciliation decision.
- First CI exposed two integration issues before N3.1 acceptance: an Arabic FTS assertion assumed stemming that `unicode61` does not provide, and the inherited selected-production-route job lacked the new pinned vcpkg setup. Corrected the fixture and configured that job with the exact baseline/toolchain; reruns are green. N3.1 storage/schema is verified, including the corrected runtime linkage in local and final Windows CI Debug/Release builds. N3.2 safe scanning is active. Search currently matches indexed tokens and does not provide Arabic morphological stemming.
- Started N3.2 with a path-only scanner and streaming native directory enumerator. It publishes bounded batches, supports cancellation and explicit unavailable/partial/limit outcomes, skips symlink entries and enforces depth/directory/file caps. Added deterministic fixture-based coverage plus a real Unicode-path filesystem smoke test; local and Windows CI verification pass. Scans do not mutate the index or remove missing records.
- Added explicit multi-root scanning at `609c8b9`: overlapping and nested roots publish each physical PDF path once, duplicate suppression is reported, root IDs are validated before scanning, and an unavailable root keeps its own outcome without invalidating successful roots. Local Visual Studio 2026 Debug and Release builds each pass 20/20 tests; final branch Debug/Release and inherited PDF checks also pass.
- Activated the database's scan-generation safety fields at `352c5d0`. Every root scan receives a durable generation; superseded scans cannot mark locations or overwrite newer outcomes; only a complete scan records a new completion time. Partial, interrupted and offline scans preserve the last trusted completion evidence and all indexed books. Local and Windows CI Debug/Release suites pass.
- Integrated bounded scans with durable root state at `38d3cab`. Existing locations are marked in the active generation; unknown paths stream forward for later identity inspection without creating or relinking documents; overlapping unknown paths are published once. Complete, partial, offline and cancelled outcomes update only the state they can prove. Local Visual Studio 2026 Debug and Release suites pass 20/20 each; final branch Debug/Release and inherited PDF checks pass.
- Added Qt PDF-backed library inspection at `30137a1`. Discovered files now have explicit readable, password-locked, malformed/unreadable, unsupported-security and missing states, with filename/metadata, page count and filesystem revision signals collected where available. The inspector never supplies or bypasses a password. Local Debug and Release suites pass 21/21 each; final branch CI is pending.
- Started N3.3 with an engine-independent identity policy and ADR-0005. Atlas may automatically relink a move only when a complete scan proved the old path missing and filesystem identity is preserved; surviving originals remain distinct copies, content-only matches remain ambiguous, and offline or incomplete roots never authorize guesses. Local Visual Studio 2026 Debug and Release suites pass 22/22 each. Durable reconciliation proposals/apply and restart coverage remain before this subgate is complete.
- Added SQLite schema v4 and durable reconciliation at `f278bec`. Atlas records completed scan generations and opaque filesystem identity, persists a reviewable pending proposal without changing the book, and revalidates every safety condition transactionally before a proven move is applied. Restart tests prove stable document identity and audit state; ambiguous offline proposals cannot apply and can be explicitly dismissed. Local Debug and Release suites pass 22/22 each. Native filesystem identity collection and remaining integration/owner evidence are still required.
- Added the native filesystem identity adapter at `fb84173`. On Windows it records the volume/file identifier via read-attributes-only handles with read/write/delete sharing; the portability boundary also has a POSIX device/inode implementation. Real filesystem tests prove stability across rename, a different identity for a byte-identical copy, Unicode paths and explicit missing-path state. Local Debug and Release suites pass 23/23 each.
- Integrated native identity with bounded scanning at `0ee0237`. Known paths initialize or confirm their stored native identity; a different filesystem object at the same path is emitted as a replacement candidate carrying the preceding Atlas document ID and is not silently marked current, reassigned or deleted. Local Debug and Release remain 23/23 green.
- Added the safe ingestion orchestrator at `8516cd0`. Completed discoveries now become either newly inspected stable documents or durable pending reconciliation proposals; proposals are never applied implicitly. A real Windows filesystem flow proves new ingestion, Unicode rename preview, explicit move application with the same Atlas ID, distinct byte-identical copies and ambiguous same-path replacement. Local Debug and Release suites pass 24/24 each.
- Delivered the real N3 Library workspace through `9753030`: background folder ingestion, safe search, per-folder/list/grid views, Favorites, Recents, explicit file-change review, English/Arabic RTL, keyboard shortcuts and plain unavailable/error states. Local Debug/Release remain 24/24.
- Replaced dated default controls with shared restrained button/input primitives and completed an actual rendered review through `5e15fb2`. The review removed a duplicated Add folder action and noisy ready banner, corrected navigation alignment/focus treatment, and verified English wide, 760 px narrow, Arabic dark and 200% layouts.
- Moved Library queries, mutations and scan preparation onto a serialized background worker at `d14f1ea`; the interface thread no longer performs SQLite work. Added a repeatable N3 Release benchmark at `dd47f4a`; on the named HP Victus/SK hynix NVMe fixture, 1,000 Unicode PDFs scanned in 1 ms, indexed in 541 ms and searched at 707 us p95 across 200 samples. Slower/removable storage remains an explicit owner test.
- Added exact N3 owner-package staging to Windows CI and a plain-language owner checklist. Automatic folder watching is not falsely claimed: N3 uses Add folder/Rescan; bounded/coalesced watching is assigned to N7 if introduced.
- The first exact-head strict Debug CI build exposed MSVC `C4702` inside Qt 6.10.3 generated QML cache code when `/WX` was enabled. Kept warnings-as-errors for Atlas sources and narrowly suppressed that upstream/generated warning on `atlas_reader`; a local `/WX` rebuild then passed before the CI retry.
- Closed the N3 implementation/automation gate at `acfe8df8f18ba59500d7a786c93bdf6df96cb16d`. Windows branch run `36290055257` and PR run `36290052467` pass Debug and Release, while selected-route, coordinate, fidelity and stress workflows pass on the same implementation head. GitHub artifact `10921014058` contains the exact owner package; its 40,056,052-byte ZIP has SHA-256 `6226b038a990497fe1799b3c500ac3e4273db2af0755b238b055c756c227037c`.
- Moved N3 to **Ready for owner test**, not Accepted. The owner must still complete the physical offline/removable-root, keyboard/Narrator, mature-product comparison and real-library checks and record explicit `N3 PASS`; N4 remains closed.
- A final documentation-head Release rerun exposed a nondeterministic parallel-build race: three inherited PDFium qualification probes could deploy the same `pdfium.dll` simultaneously and one copy received `permission denied`. Serialized those three Windows probe deployment targets while preserving parallel compilation elsewhere; this is build reliability, not a product-scope change.
- Added the owner-selected Atlas Reader application mark as a compressed transparent 1024 px master and a 99,055-byte Windows ICO with 16–256 px frames. Wired the ICO into the native executable resource and recorded source provenance, checksums and modification rules; the brand artwork does not change the restrained in-product UI language.
- Configured this PC for native work: added the installed Visual Studio CMake bin to the current user's PATH, bootstrapped vcpkg at the exact pinned baseline, and installed Qt 6.10.3 MSVC 2022 x64 with Qt PDF. Added the VS 2026 configure/build/test preset without changing the VS 2022 CI baseline. The initially self-referential `CMAKE_PREFIX_PATH` preset macro was corrected to inherit the parent environment. Local fixture setup now follows the same generators/validator as CI; full local Debug and Release builds each pass 20/20 tests.
- Local and GitHub link logs exposed MSVC `LNK4098` on SQLite test executables because `x64-windows-static` used a static CRT against Qt's dynamic CRT. Switched to the pinned vcpkg `x64-windows-static-md` triplet across presets, CI, and dependency/build documentation. Corrected local Debug/Release builds and final GitHub PR/push checks each pass all 20 tests; the final GitHub logs contain no linker warning. N3.1 remains verified and N3.2 scanning remains active.
- Updated live status pages and the document checker for checkpoint transitions; closed N2 records retain their historical `N2|accepted` status.
- Corrected the first N3 owner-test findings without opening N4: the Windows app target now launches as a GUI application without a background console; implausible embedded PDF metadata such as `1.docx`, raw `<...>` hex strings and old source paths falls back to the current PDF filename; and Library card actions now keep a consistent safe inset in wide, narrow and Arabic/RTL layouts. Clarified that N3 reviews only identity-sensitive moves/replacements, while first-page cover thumbnails remain an explicit N7 capability. N3 stays Ready for owner retest until the owner confirms the corrected package and completes the skipped comparison check.
- Corrected the second N3 owner-test metadata hierarchy after real PDFs exposed plausible but misleading embedded titles (`Binde.pdf`, `TIFF.pdf` and a wrapped PDF title). Library cards now use the real filename stem as the large primary name, show a distinct embedded/user-set document title underneath, show known author and actual extension as secondary information, and suppress generic unknown-author filler. The extension badge/type is data-driven rather than hardcoded for PDF, establishing the same protected-extension behavior for future EPUB and other admitted formats. The binding scope now requires future normal renames to edit only the stem and preserve the extension; metadata edits remain separate.
- Aligned normal Library, folder and Favorites ordering with the visible filename instead of hidden embedded title metadata. The shared filename/metadata/type contract explicitly covers EPUB, AZW3, KFX, MOBI, PRC, CBR and CBZ as future admitted formats without claiming that N3 can discover or open them; N3 remains PDF-only.
- Repaired this PC's native environment after owner retesting exposed a stale invalid Visual Studio Build Tools CMake PATH entry and a `VCPKG_ROOT` incorrectly aimed at Qt. The saved user environment now points to Visual Studio Community CMake 4.2.3, this checkout's pinned vcpkg tree and Qt 6.10.3; fresh Debug and Release runs each pass 24/24 tests without missing-Qt-DLL dialogs.
- Closed the previously vague post-PDF format gap with a dedicated roadmap and named checkpoints: non-DRM EPUB first, CBZ/CBR second, non-DRM AZW3/MOBI/PRC third, and KFX research-only until a lawful maintainable route is proven. Each track now has security, accessibility, RTL, performance, interoperability, preservation and explicit owner-PASS gates; this planning does not expand the current PDF-only N3 implementation or Windows 2.0 scope.
- Made format readiness a binding N4–N11 review rule: shared book identity, Library, metadata, search, progress, backup and UI models must avoid accidental PDF-only assumptions, while PDF rendering, coordinates, security and writes remain correctly isolated and rigorously PDF-specific. Added F1–F3 and the non-promissory KFX research gate to the release ledger.

### N2 owner acceptance — 2026-09-27

- Moved seven PDF regression workflow push triggers from the retired N2 branch to permanent `main`, preserving their PR triggers. Performance and qpdf qualification retain PR/manual triggers. This keeps accepted PDF checks active after branch cleanup.

- Rewrote the public README with a reader-focused introduction, explicitly planned benefits, honest current availability, next stages and ways to follow the project. Added `docs/DEVELOPER_GUIDE.md` for technical orientation and linked the existing authoritative build, architecture, QA and evidence documents. No product scope or acceptance requirement changed.

- Adopted permanent branch names at the owner's request: native development is `main`, and the discontinued Flutter backup is `legacy/flutter`. Renamed both existing branches without rewriting commits; synchronized local tracking, public notices and workflow documentation. Future checkpoints do not rename either permanent branch.

- Merged N2 PR #4 after all 20 final-head checks passed. Removed four obsolete remote branches and the completed local N2 branch; preserved two unmerged historical branches through explicitly non-release `archive/*` tags. Only native integration and the obsolete Flutter backup remain. Exact recovery references and worktree purposes are recorded in `docs/baselines/N2_ACCEPTANCE.md`.

- Repository landing page and default file tree now target native development (`main`); Flutter `legacy/flutter` is a secondary obsolete backup/reference, with development stopped.

- Removed unreferenced `DO_NOT_USE.tmp` and obsolete encrypted A013 PDF/generator; canonical A013 is image-only and encrypted-write coverage uses manifest/CI-qualified A015. Deleted files remain recoverable from Git history.
- Closed the superseded Flutter roadmap PR #2 after publishing the discontinued-development notice.

- Owner explicitly tested and approved N2 with `N2 PASS`; ADR-0004 is Accepted.
- Added `docs/baselines/N2_ACCEPTANCE.md` with evidence identity, integration route, future obligations and the stop before N3.
- Corrected Git policy: native checkpoint PRs target `main`; `legacy/flutter` retains the discontinued Flutter reference.
- Published that Flutter development has stopped and C++/Qt native development is active.
- At N2 closeout N3 was Not started. The engineering version remained `2.0.0-alpha.2`.

- Audited every tracked Markdown plan/evidence file for status, route and
  checkpoint-ownership drift; corrected the live N2 state and the final Qt
  PDF/qpdf responsibility route without rewriting historical measurements.
- Added a binding N0–N11 QA/owner-test matrix, assigned every N2 limitation to a
  named implementation checkpoint and release blocker, and filled missing QA
  gates for N3–N11 including Windows integration, settings and diagnostics.
- Added an explicit Git/documentation workflow plus automated Markdown link,
  live-status and checkpoint-gate validation in CI.

### N2 — PDF engine qualification spike (`2.0.0-alpha.2`)

- N1 was explicitly Accepted by the owner on 2026-09-22 and merged into the accepted integration line at `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`.
- Opened dedicated branch `native-v2-n2-pdf-engine-qualification` from that exact accepted N1 state; `main` remains untouched.
- Advanced native prerelease identifier from `alpha.1` to `alpha.2` without adding a production PDF engine dependency.
- Added `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` defining N2 scope, exclusions, phases, normalized responsibilities, fixture classes, performance rules, hard blockers and owner stop gate.
- Added `docs/baselines/N2_PDF_ENGINE_MATRIX.md` as the binding evidence table. Untested or undecided capabilities remain explicit rather than guessed.
- Added Proposed `ADR-0004-pdf-engine-responsibilities.md`; its frozen decision now selects official dynamic Qt PDF 6.10.3 plus the qpdf 12.4.1 CLI, but it cannot become Accepted before final strict CI and explicit owner `N2 PASS`.
- Added deterministic PDF fixture provenance/privacy/checksum/mutation rules under `tests/fixtures/pdf/`.
- Updated `AGENTS.md` so coding agents operate under N2 boundaries and cannot drift into production Reader, SQLite/index, bookmarks, annotations, migration or installer work.
- Refreshed dependency/upstream catalogs with Qt PDF, PDFium and qpdf qualification constraints.
- Preserved the split-engine possibility: read/render/search/navigation and structural/security/transformation responsibilities may be assigned separately behind Atlas-owned normalized contracts.
- Qualified Qt PDF 6.10.3 core read/text/render behavior on the deterministic corpus without linking Qt PDF into the product shell/domain boundary.
- Qualified PDFium `156.0.8066.0` / `chromium/8066` through a checksum-pinned non-V8 Windows x64 probe package; retained the upstream rule that public PDFium API execution must be serialized.
- Qualified outline/link semantics in both read engines; recorded Qt's duplicate raw external-URI row and required Atlas semantic de-duplication.
- Added deterministic A006 Unicode evidence covering Arabic, tashkīl, mixed Arabic/English, Urdu and Unicode outlines/search. Both engines preserve tested code points but require Unicode normalization before equality/index/search comparison for the vocalized source-order case.
- Added malformed/password fixtures and semantic error-state qualification for Qt PDF/PDFium.
- Added repeated Release performance probes with 3 warmups + 31 measured iterations, while explicitly avoiding a false winner claim from non-equivalent Qt/PDFium search execution models.
- Added qpdf 12.4.1 structural/security/transformation qualification from the first-party MSVC64 package with exact asset checksum and preservation assertions.
- Qualified qpdf user/owner password state and restricted permission inspection on deterministic R2 fixtures; legacy RC4 exists only as test data.
- Qualified qpdf no-op rewrites and controlled ASCII/Unicode existing-outline title mutation while preserving explicit unrelated-document invariants.
- Recorded that qpdf may renumber indirect objects, so PDF object numbers are not Atlas portable identity.
- Added A010 synthetic `/Sig` + `/DocMDP` evidence and established the signed/certified pre-mutation interlock: Atlas must detect before mutation and must never equate surviving signature dictionaries with preserved cryptographic validity.
- Added A011 rendering-fidelity evidence covering effective CropBox geometry, 90° rotation, semantic vector landmarks at 1×/2×, annotation off/on behavior and engine-specific native blank backgrounds. Atlas owns background/compositing policy.
- Added the Atlas portable page-space contract: effective visible page, upper-left origin, X right, Y down, units PDF points.
- Qualified link/search rectangle normalization across normal, cropped and rotated pages; recorded that Qt raw rotated search rectangles can have negative width and must be canonicalized.
- Added A012 explicit `/XYZ` destination evidence. Normal targets pass in both engines; PDFium directly normalizes the 90° target, while Qt requires structural rotation metadata external to `QPdfLink` to complete portable destination normalization.
- Added dedicated stress/concurrency evidence: 500 complete open/render/text/close lifetimes per read engine and a 4-producer × 250-request PDFium queue with exactly one PDFium worker/API lane. All 1,000 queued jobs completed, producer-side PDFium calls remained zero, maximum active PDFium API execution remained one, and shutdown was clean.
- Recorded working-set trends from the 500-cycle stress run without inventing a universal leak threshold. Qt showed a larger upward working-set trend than PDFium in this one synthetic hosted-runner series; it is retained as follow-up evidence rather than declared a leak.
- Added `docs/baselines/N2_STRESS_CONCURRENCY.md` with exact stress implementation/run/artifact identities and architectural serialization conclusions.
- Added `docs/baselines/N2_PRODUCTION_DISTRIBUTION.md` separating qualification packages from shippable routes: official dynamic Qt if selected; Atlas-owned pinned upstream PDFium source build if selected; first-party qpdf CLI process boundary as the current lower-coupling structural route.
- Recorded production compliance requirements for exact provenance, runtime-file inventory, license/NOTICE/SBOM bundle, security-update ownership and rollback before any selected component is promoted to a release dependency.
- Closed B1 image-only rendering, B2 real-font Arabic/Urdu rendering, B3 full outline mutation breadth, and B4 permitted encrypted-write preservation with bounded CI evidence.
- Repaired A015's invalid missing resource dictionary, added strict source validation, and corrected the qpdf permission assertion to read the JSON v2 `encrypt.parameters.P` field.
- Froze the selected production route as official dynamic Qt PDF 6.10.3 for read/render/text/navigation plus the first-party qpdf 12.4.1 CLI for structural/security/write; standalone PDFium remains qualification-only because an official Atlas-controlled source-build route was not frozen.
- Added selected-route CI that disables standalone PDFium, runs the Qt PDF and six qpdf regression slices, stages and smoke-tests the actual Windows runtime, enforces qpdf file hashes, rejects `pdfium.dll`, and emits deployed-file, SPDX, license, attribution, update and rollback evidence.
- Measured the staged qualification package at 64,069,704 bytes across 30 files: 54,965,368 bytes for the deployed Qt/read route and 9,104,336 bytes for qpdf.
- Strict CI passed on the synchronized implementation/documentation head; owner review and explicit `N2 PASS` followed on 2026-09-27.
- Kept N3 and all later product-feature checkpoints closed until N2 is explicitly Accepted.

### N1 — Windows toolchain + empty-shell baseline (`2.0.0-alpha.1`) — Accepted

- N0 was explicitly Accepted by the owner on 2026-09-22.
- Created dedicated branch `native-v2-n1-toolchain-baseline`.
- Advanced native prerelease identifier from `alpha.0` to `alpha.1`.
- Aligned local CMake presets with the Visual Studio 17 2022 x64 generator used by CI instead of using a separate Ninja path.
- Added a Debug + Release Windows CI matrix.
- Added strict Atlas-owned compiler policy: `/W4 /permissive- /Zc:__cplusplus`, with `/WX` in CI.
- Added privacy-safe Qt logging categories for startup, UI, and performance diagnostics.
- Added local-only numeric metrics for QML load, first swapped frame, resize frame intervals, and clean shutdown.
- Added deterministic `--benchmark-shell` resize exercise and `--quit-after-ms` lifecycle test option.
- Added `--language en|ar`, `--theme system|light|dark`, and `--metrics-file` shell test controls.
- Added live English/LTR ↔ Arabic/RTL direction proof in the empty shell.
- Added light/dark shell proof without introducing a production theming system.
- Added `tools/bench/measure-windows-shell.ps1` for physical-machine startup, memory, CPU, and frame baseline measurement.
- Added ignored `artifacts/` output for machine-specific benchmark data.
- Added `docs/TOOLCHAIN.md` and N1 baseline/qualification records.
- Added ADR-0003 documenting the reproducible public Qt 6.10.3/MSVC 2022 alpha pin.
- Added a self-contained Release qualification artifact produced with `windeployqt` so owner hardware testing does not require a local compiler/Qt install.
- Added CI syntax/runtime validation for PowerShell qualification scripts before packaging.
- Physical testing on Windows 11 / Core i7-14650HX / RTX 4070 Laptop / 144 Hz verified English/LTR, Arabic/RTL, mixed script, light/dark, 100%/200% scale and clean startup/shutdown.
- Initial physical measurements exposed a `WaitForInputIdle` sampler defect and empty-shell startup around 1.8–2.2 seconds, so N1 remained open while the issue was attributed instead of normalized.
- Reworked the physical harness to wait for Atlas's own first-frame metric, reject dead-process samples, capture power/storage/DPI context, preserve packaged commit identity, and report startup p50/p95.
- Added staged startup instrumentation around application, QML engine/load and first frame.
- Switched the N1 shell to compile-time `QtQuick.Controls.Basic` while retaining Atlas palette/RTL behavior.
- Isolated first meaningful text/font initialization as the dominant Windows startup cost and compared Windows font engines on a temporary diagnostic branch.
- Adopted `fontengine=gdi` through packaged `qt.conf` for the N1 Windows shell after measured comparison; documented it as a qualified Windows decision with later re-evaluation conditions rather than a universal permanent claim.
- Final Arabic warm first-frame p95 measured `772.875 ms`.
- English 22-run confirmation measured warm p50 `365.941 ms`, p95 `1014.827 ms`, with a temporary system-wide slow cluster and recovery; owner explicitly accepted this measured English startup tradeoff.
- Fixed a focused keyboard activation defect discovered before acceptance by handling Enter/Return on the N1 buttons; owner physically retested and passed the correction.
- Accepted user-qualified runtime commit: `73f567cf2c557f185371d7f944ebf6d69453105a`, artifact digest `sha256:0fa2b638c5e17e90a30f43c117f4c5f74b509fade31108cfe9119e7a86c17d88`.
- Owner explicitly recorded `N1 PASS` on 2026-09-22.
- Added the repository rule: **a passed checkpoint cannot be reopened without new evidence of a user-facing regression.**
- Merged N1 PR #3 into `native-v2-bootstrap` at `f8bdc2e5bc74ccb94d593e7a7e5307ae6163afe2`.
- PDF engines, qpdf, SQLite/FTS5, scanner, production Library/Reader/Bookmarks, annotations, migration, and installer implementation remained outside N1.

### N0 — Native 2.0 bootstrap program (`2.0.0-alpha.0`) — Accepted

- Established C++23 + Qt Quick/QML + CMake successor architecture.
- Added pure C++ contracts for document capabilities, PDF engine, filesystem, and library index.
- Added minimal Qt Quick application shell and native smoke test.
- Added Windows CI and corrected its public Qt/toolchain configuration after early bootstrap runs exposed reproducibility issues.
- Added project-local CodeGraph and GitHub connector/MCP configuration.
- Defined the native release line through `2.0.0`: N0–N2 alphas, N3–N9 betas, N10 RC, N11 stable Windows 2.0.
- Expanded `CHECKPOINTS.md` into explicit subgates, owner evidence, and release mapping.
- Added complete Windows 2.0 product scope with P0/P1/P2/deferred boundaries.
- Added open-source dependency/tool/plugin policy including vcpkg manifest direction, PDF-engine qualification, testing/profiling/accessibility tooling, AI-agent guardrails, and a concrete upstream project catalog.
- Added quality/testing/preservation/failure-injection strategy.
- Added security/privacy threat model and vulnerability-reporting policy.
- Added UX/accessibility/Arabic/RTL design contract.
- Added competitive baseline/differentiation strategy covering mature PDF readers/editors without turning feature-count parity into the product goal.
- Added release strategy, ADR process, vcpkg dependency ADR, and development workflow documentation.
- Expanded licensing/third-party/SBOM guardrails.
- Added measurable provisional performance budgets and benchmark hygiene.
- Added Atlas-owned data/format contracts covering document identity, destinations, bookmark overlay states, versioned interchange, backups, and migration boundaries.
- Added a product/engineering success scorecard so beta progression is measured by safety, correctness, responsiveness, accessibility, interoperability, and resource use rather than feature count.
- Added a living risk register for engine, corruption, licensing, C++ safety, portability, accessibility, migration, packaging, scope, and performance risks.
- Added `docs/N0_HANDOFF.md` as the explicit owner-review/acceptance checklist.
- Explicitly kept production PDF engines, SQLite, scanning, reader features, bookmarks, annotations, migration, and installer code out of N0.

### N0 CI findings and verification

Early CI runs exposed two useful setup defects:

1. Public `aqtinstall` could resolve Qt 6.11.2 but could not locate its Windows repository XML, so public CI moved to a reproducibly installable Qt 6.10.3 pin.
2. `CMakeLists.txt` incorrectly hard-required the exact Qt 6.11.2 patch while CI intentionally used a compatible patch. Application source now states the Qt 6.10 API floor while exact patch selection belongs to the toolchain policy.

The Windows runner was pinned to Windows 2022 with the Visual Studio 17 2022 x64 generator to match the MSVC 2022 Qt kit instead of inheriting changing `windows-latest` toolchains.

N0 passed branch-head Windows Qt installation → CMake Configure → Release Build → CTest and was explicitly Accepted by the owner on 2026-09-22.

## Versioning

The native successor belongs to the `2.0.0` release line.

- `2.0.0-alpha.0` — N0 bootstrap — **Accepted**
- `2.0.0-alpha.1` — N1 Windows/toolchain baseline — **Accepted**
- `2.0.0-alpha.2` — N2 PDF-engine qualification — **Accepted**
- `2.0.0-beta.1` onward — usable feature checkpoints from N3
- `2.0.0-rc.N` — release qualification
- `2.0.0` — accepted stable Windows release

See `docs/RELEASE_STRATEGY.md` for the binding rules.

A failed operating-system replacement is now hash-checked before clearing its
journal: if the destination differs from the original, the result is recovery
required and the backup/journal remain. A regression simulates a destination change
at this failure boundary. This avoids treating the OS error code as proof that
nothing changed.

N5 language review in progress: simplified Urdu import/export and bookmark-destination labels, standardized Arabic bookmark wording, corrected the font contract, and added explicit language/rendering/menu-size closure gates. Runtime qualification remains open.

- N5 UI repair: keep an explicit system font for Latin text, preserve the bundled Arabic/Urdu fonts, and size bookmark-menu rows from shaped text instead of the Basic style background minimum. Constrain overflow to the window with a visible scrollbar; wider language/size qualification remains open.

- Clarified N7 library scope after owner feedback: covers, selectable title/filename display (book-title default), separate local/PDF metadata editing, and extension-safe file rename now have explicit N7-LIB-01–04 requirements and tests. These are planned, not features in the N5 build.

- Repair library grid card height using Arabic/Urdu/Latin font metrics so the action row has reserved space; add a grid QA entry point. Load Preferences on request to reduce startup work, preserving its window-relative placement. Startup remains above target. Urdu Preferences now uses the familiar سیٹنگز label.

- Startup: the bundled Arabic/Urdu fonts are registered once before the interface loads, instead of from QML after text had already used the font list (which made Windows re-read every installed font). Windows startup is now measured in CI.
- Safe save: a full backup drive no longer locks the book behind a false "interrupted save"; a save cut off by an unplugged drive settles by itself when the drive comes back unchanged.
- Screen readers: bookmark rows are announced as a tree (level, open/closed, page, state); option buttons, check boxes and every menu command are reachable. A new automated accessibility check covers ten screens in three languages.
- Arabic and Urdu wording made consistent (zoom vs window size, "protected" vs "saved", one spelling of PDF and bookmark).
- Urdu library sidebar rows no longer overlap.
