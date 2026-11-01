# N4 Native Reader Foundation Evidence

<!-- atlas-status: N4|in-progress -->

## Process change — 2026-11-01

The owner chose **batched reviews** and a **focused 2.0** ([PLAN.md decision log](../../PLAN.md#9-decision-log)). Sub-stages now close on automated evidence plus a recorded rendered self-review; the owner reviews once at the end of N4 using [N4_REVIEW.md](../N4_REVIEW.md). N4.1 therefore closes as Implemented and automated-Verified, and **N4.2 is the active sub-stage**. The N4.1 rendered owner items (English/Arabic/RTL, 200%, keyboard/Narrator, restart/password, file release, source preservation) are carried into that review and still block `N4 PASS`. Side-by-side reading (C17) and the simplified text view (C18) left N4 scope and moved after 2.0. Sections below that mention a separate `N4.1 PASS` describe the earlier process and are kept as a historical record.

## Opening record — 2026-09-28

- Accepted predecessor: N3 `2.0.0-beta.1`.
- Base merge: `739508e3e698016e8a6a4cb8b87c2b4aa418d66d` on `main`.
- Active branch: `native-v2-n4-reader-foundation`.
- Planned version: `2.0.0-beta.2`; the source remains `2.0.0-beta.1` until N4 earns promotion.
- Starting state: no N4 reader implementation or result is claimed.

## Subgate ledger

| Subgate | Status | Implementation SHA | Automated evidence | Rendered/owner evidence | Open blockers |
|---|---|---|---|---|---|
| N4.1 Session/open model | **Implemented; automated Verified** | `5ac63df7da45d711380cfaac0e83f0b96deda7f4` | Windows CI run `36690884159`: Debug + Release build/test PASS, 43/43 tests; `atlas_reader_controller` PASS; inherited Fidelity `36690884055`, Stress `36690884080`, Coordinates `36690883965`, Selected Production Route `36690883948` PASS | owner checks folded into the end-of-N4 review ([guide](../N4_REVIEW.md), checks 1–4 and 9) | Narrator naming of the password field and Windows 200% scale remain unverified until that review |
| N4.2 Virtual viewport | **Implemented; automated Verified** | `da5e512` | Windows CI run `41242163280` (MSVC 2022, Qt 6.10.3, `/WX`): Debug + Release **45/45** including `atlas_reader_viewport` and `atlas_reader_viewport_render`; N2 regression workflows pass; also PASS locally on Linux/Qt 6.4 with ASan/UBSan for the core | Rendered self-review: English light and Arabic dark at 1260 × 800 (Qt 6.4 / Xvfb) | Tiling for pages beyond 8192 px; whole-file-in-memory session (memory = file size) |
| N4.3 Foxit workspace, design system, three languages | **Implemented; automated Verified** | `2a5ab85` (signed) | Windows CI `41281081373` green (MSVC 2022, Qt 6.10.3, `/WX`); string check PASS (185 keys × EN/AR/UR) | Rendered self-review EN/AR/UR × light/dark (Qt 6.4 / Xvfb); review build delivered to `review-builds/N4.3-cc197f0` | Window move/snap/resize on Windows and Narrator/200% in the end-of-N4 review |
| N4.4 Navigation | **In progress** (slice 1 implemented) | this commit | `atlas_reader_viewport` (ViewHistory), `atlas_reader_viewport_render` (Previous/Next View) PASS on Linux/Qt 6.4; Windows CI pending | Pages panel EN/AR rendered (Qt 6.4 / Xvfb) | Layouts, Fit Visible/Zoom To, page labels, Reverse View, AutoScroll, tiling |
| N4.5 Outline/text/search/links | Not started | — | — | — | normalized reader adapters not implemented |

## Required final evidence

- Debug/Release test totals and exact GitHub run IDs at the proposed head.
- Package name, artifact ID/digest, inner ZIP SHA-256, build identity and clean-profile smoke.
- Source PDF before/after hashes proving N4 read-only behavior.
- Page geometry/rotation/link/search fixture results.
- 2,000-page and multi-tab memory/cache series with named hardware.
- Time-to-first-page, page-jump, scroll and search latency distribution.
- Rendered English/Arabic, light/dark/high-contrast, wide/narrow and 100%/200% review.
- Keyboard and Narrator/UIA route.
- Mature-reader workflow comparison and defect disposition.
- Owner checklist and explicit `N4 PASS`.

N4 is **In progress**, not Verified, Accepted or Released. PR #6 stays Draft until the owner records `N4 PASS`.

## N4.1 implementation checkpoint — 2026-09-30

Implemented without beginning N4.2:

- an engine-independent read-session model with explicit loading, ready, password-required, missing, malformed, unsupported-security, cancelled and failed states;
- duplicate-path activation instead of duplicate tabs, deterministic activation/close behavior, reopen revisions and rejection of stale or post-close asynchronous results;
- background Qt PDF inspection for direct local files and Library Open actions, returning only Atlas-owned metadata to the interface;
- an initial tab strip and honest in-app ready/error surface; no page-rendering claim is made before N4.2;
- optional local tab restoration, disabled by default, storing paths only and never passwords;
- direct PDF opening through `Ctrl+Shift+O`, tab close through `Ctrl+W`, and a route back to the accepted Library;
- password submission through an obscured field, specific wrong-password retry state, successful unlock through Qt PDF, and field clearing after submission;
- password non-persistence: restart restores only source paths and a protected document returns to password-required state;
- cancellation ownership: closing/retrying retires the superseded worker, prevents stale results from mutating the session, keeps the worker owned until completion, and waits for active/retired workers during controller shutdown;
- revision/watcher-specific cleanup so an old completion cannot erase a newer retry watcher;
- corrected Reader-body session actions so retry/close target the actual session delegate rather than an intermediate layout object.

### Automated Windows evidence

Implementation head: `5ac63df7da45d711380cfaac0e83f0b96deda7f4`.

Windows CI run `36690884159` on Windows Server 2022 / MSVC 2022 / Qt 6.10.3:

- Debug Configure: PASS;
- Debug Build: PASS;
- Debug CTest: **43/43 PASS**;
- Release Configure: PASS;
- Release Build: PASS;
- Release CTest: **PASS**;
- Release artifact/evidence staging: PASS.

The new `atlas_reader_controller` integration test passes and covers:

- ordinary asynchronous open to ready state and page count;
- ready-session Windows rename/move proof, demonstrating the inspection path does not retain an unnecessary file handle;
- encrypted open → password-required;
- wrong password → explicit retry state;
- correct password → ready;
- correct and rejected passwords absent from the persisted `reader-session.ini`;
- opt-in path-only session restoration across a fresh controller instance;
- protected restored session asks for the password again;
- repeated close-during-open cancellation;
- controller shutdown waits for retired workers and the PDF can be renamed after shutdown, proving no orphan worker retains the Windows file handle.

Inherited exact-head regressions also pass:

- N2 PDF Fidelity run `36690884055`;
- N2 PDF Stress run `36690884080`;
- N2 PDF Coordinates run `36690883965`;
- N2 Selected Production Route run `36690883948`.

The earlier local Visual Studio 2026/Qt 6.10.3 Debug evidence remains useful historical evidence but is superseded for N4.1 closure by the exact implementation-head GitHub Debug/Release evidence above.

## N4.1 remaining owner gate (superseded on 2026-11-01; see Process change)

No automated evidence can replace the actual rendered Windows review. The owner must complete `docs/N4_1_OWNER_TEST.md`, covering the N4.1 surface in English/Arabic, RTL, 100%/200%, keyboard/Narrator, password/restart behavior, close-during-open behavior, file release and read-only/source-preservation observations.

N4.1 remains **Ready for owner test**, not Accepted, until defects from that review are fixed/retested and the owner records `N4.1 PASS`. N4.2 must not begin earlier.

## Review preparation and local setup repair — 2026-10-16

The active source checkout is now `native-v2-n4-reader-foundation`. The old `atlas_native_v2_n2` folder retained an incomplete dependency directory; the saved user `VCPKG_ROOT` still targeted it. Restored vcpkg under the active checkout's ignored `build/vcpkg`, checked out the exact declared baseline `9e2895bf6afb246396d85232ba70fcfa1fa67ba1`, bootstrapped it with metrics disabled and updated the saved user setting. Qt stays at 6.10.3; CMake/CTest resolve to the installed Visual Studio tools. `tools/dev/enter-windows-environment.ps1` now validates the kit, executable/toolchain files and baseline before setting the current process environment. No product dependency pin or accepted CI toolchain changed.

### Verification and corrections

- Clean local VS 2026 / MSVC 19.51.36246.0 / SDK 10.0.26100.0 configure and Debug/Release builds pass with warnings as errors.
- Local production-route CTest: **26/26 PASS in Debug and Release**. PDFium probes are disabled locally; the historical 43-test CI total includes the extra comparison probes and must not be substituted for these local totals.
- Extended the controller test to prove disabled-restoration restart, rejection of an HTTPS open request, filename-first presentation, separately retained document title and identical SHA-256 source bytes after ordinary/protected open, retry, restart, rename and cancellation/shutdown.
- Actual rendered review covered English light at 1260 × 800 and Arabic dark at 760 × 560 logical pixels. Additional English/Arabic dark screenshots use `QT_SCALE_FACTOR=2` at 760 × 560 logical pixels. Controls fit without clipping. This simulated Qt scale check does not replace the owner's Windows display-scale/Narrator review.
- Live Windows input verified Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+W and wrong/correct public fixture passwords submitted with Enter. UI Automation exposes named tab items, per-file close actions, Library/Open actions and restoration control; password text is masked.
- The inspection tool did not expose a password-field name or a reliable focused-element report. Password keyboard submission worked, but **Narrator naming/focus remains unverified** and must pass owner checklist 8 before N4.1 acceptance.
- Review corrected Reader primary names to filename stems while keeping embedded titles secondary, added keyboard/accessible tab selection and selected-tab visibility, kept close controls inset, removed the stale N3 UI label and replaced viewport implementation jargon with plain review-stage text.
- Raw screenshots and local test output stay under ignored `artifacts/n4-qa/`; no personal PDFs or profile data are committed.

### Delivery contract

`tools/release/package-n4-review.ps1` stages a separate N4.1 review ZIP with the dynamic Qt/PDF runtime, restored accepted font configuration, public fixture/provenance files, build/executable identity, notices, installed Qt SBOM documents and file/ZIP checksums. Its startup smoke uses a separate profile and removes Qt/plugin/import environment paths from the child environment. Existing output directories are preserved.

The N4 branch's Release CI now uploads the N4.1 review ZIP for 30 days. It no longer labels N4 builds as accepted N3 packages; the published N3 release remains immutable. The plain-language guide is `docs/N4_1_QUICK_START.md`. Package identity/checksum and current GitHub run results are recorded in the delivery follow-up after the review package is built. PR #6 stays Draft and unmerged, the source version remains beta.1 and owner `N4.1 PASS` is pending.

## N4.2 log

### Core viewport model — 2026-11-01

Pure C++ (`atlas_reader_viewport`, no Qt or engine types):

- `ViewportLayout`: continuous single-column geometry in logical px from page sizes in points; page `/Rotate` combined with a user view rotation; centred narrower pages; margins and gaps; binary-searched `pagesInBand`/`pageAt`; page-jump offsets; fit-width and fit-page zoom; zoom clamped to 5%–6400%.
- `planRenders`: visible pages first, then a configurable prefetch band above and below, each ordered by distance from the viewport centre; scale buckets in quarter-octave steps that never render blurrier than requested; huge pages capped to an 8192 px texture side (tiling is a later step).
- `RenderScheduler`: bounded concurrency; each new plan cancels in-flight work that is no longer wanted and replaces the queue, so a fast jump starts the new page immediately; late results from cancelled tickets are ignored; cached pages are skipped.
- `PageTextureCache`: byte-budget LRU; visible textures pinned and never evicted; stale document revisions dropped on reopen; best-available lower-resolution texture for instant feedback while a sharper one renders.

`tests/reader/viewport_tests.cpp` covers rotation, layout, band queries, fit zoom, buckets, planning order, cancellation after a jump, cache eviction/pinning/reuse, and a **2,000-page memory series**: reading through a 2,000-page book at 150% zoom on a 2× display rendered 858 textures with a peak of 236 MiB against a 256 MiB budget. Memory is bounded by the budget, not by page count (inherited N2 limitation "Qt working-set trend", core half).

### Qt render adapter and viewport item — 2026-11-01

- `PdfRenderWorker` (one per open tab, own low-priority thread): reads the PDF **once into memory and releases the file**, then renders through Qt PDF from that buffer. Renaming or moving an open PDF works and its bytes never change (asserted by `atlas_reader_viewport_render`). Annotations are drawn; the user view rotation is applied on top of each page's `/Rotate`. Cancelled tickets are skipped before and after rendering.
- `ReaderViewport` (QQuickItem, `Atlas.Reader 1.0`): executes the core plan; uploads rendered images to scene-graph textures and drops the CPU copy; draws placeholder paper and a soft shadow while a page renders, reusing a lower-resolution texture when available; hidden tabs release their textures; scene-graph invalidation and item teardown free textures on the render thread. Input: wheel scroll, Ctrl+wheel zoom anchored at the pointer, arrows, Page Up/Down, Space/Shift+Space, Home/End, Ctrl+=/−/0/1/2, click to focus.
- Reading toolbar: page field (type a number, Enter), page count, zoom out/in with percentage, Fit width (default), Fit page, and a "More" menu holding *Restore reader tabs* and *Close tab*. Scroll bars appear when needed. Status view (loading, password, errors) is unchanged for non-ready tabs.
- Passwords for unlocked PDFs are kept in memory for the tab's lifetime so pages can render; they are never persisted or exposed to QML, and are erased when the tab closes or a later open fails.
- Found and fixed during self-review: with a plain `QSGNode` root, the software scene-graph renderer drew pages at window origin and ignored clipping. A `QSGTransformNode` root fixes placement and clipping on every backend; the render test now places the viewport below a header and asserts nothing is drawn outside it.
- **Known limits (owned):** pages larger than 8192 px at the current zoom are rendered at a capped resolution until tiling lands (N4.3); the whole PDF is held in memory per open tab (fine for typical books; very large scans will be revisited with N4.5 memory measurements on Windows); text selection, search, links and outline arrive in N4.4; spreads, reading direction, history and thumbnails in N4.3.

## N4.3 log

### Workspace revamp, design system and three languages — 2026-11-01

Owner request (2026-11-01): an English-first interface that is fully usable in Arabic and Urdu, user content in any language, and a complete UI revamp using Foxit PDF Editor's design and accessibility *concepts* (no Foxit artwork, icons or files). Decision recorded in [ADR-0006](../decisions/ADR-0006-interface-languages.md) and the PLAN decision log.

- **Design system:** `Theme` singleton (colour, spacing, radii, sizes, motion, icon lookup; Touch mode and a script-height allowance for Nastaliq), `I18n` singleton (live language switch, mirrored layout, Urdu Nastaliq UI font, script-aware content font so Arabic titles stay in Naskh and Urdu in Nastaliq whatever the interface language, isolate/LTR helpers for names and paths), `AppState` (Read mode, Full screen). Shared components: `AtlasButton` (primary/secondary/quiet/danger with icon), `AtlasIconButton` (tooltip with shortcut, accessible name), `AtlasTextField`, `SidebarItem`, `TitleBar`. `Main.qml` split into `LibraryView`, `ReaderView` and `ReaderTab`.
- **Languages:** 105 interface strings in English, Arabic and Urdu (`qml/i18n/Strings.js`); `tools/i18n/check-strings.py` fails on a missing key, a placeholder mismatch or a hard-coded visible string, and runs in Windows CI (Release). The language and theme are chosen from the title bar and remembered in the profile (`preferences.ini`); `--language` and `--theme` override them for one run only.
- **Workspace:** title bar (mark, location, language, appearance, Open PDF) · tab bar · command bar (rotate, Read mode, Full screen, reading colours, More) · document pane · status bar (first/previous/page box/of N/next/last, actual size/fit width/fit page, zoom slider and percentage). Read mode (Ctrl+H) hides the chrome and shows a floating mini bar; F11 full screen, Esc exits.
- **Reading colours** Paper, Sepia, Night, High contrast are applied to rendered page images only; the PDF never changes. View rotation (Ctrl+Shift+Plus/Minus) and Foxit-compatible Ctrl+1/Ctrl+2/Ctrl+0.
- **Library restyle:** list rows (cover · title, detail, path · state · favourite · Open) and a grid of cards, both mirrored correctly in Arabic and Urdu.
- **Opening from the command line:** PDF paths passed to `atlas_reader.exe` open in the reader (Open with…, drag onto the icon).
- **Bundled assets:** Noto Nastaliq Urdu and Noto Naskh Arabic (OFL-1.1, SHA-256 in `assets/fonts/README.md`) and 41 Fluent UI System Icons (MIT), compiled into the executable; licences are copied into the review package's `notices/`. Qt Svg is linked so the SVG image plugin is deployed, and the packager checks `Qt6Svg.dll` and `imageformats/qsvg.dll`.
- Found and fixed during self-review: QML singletons were not visible to files in the `qml/` sub-folder of the module, which left every `Theme`/`I18n` value undefined; all QML files now sit at the module root through resource aliases. The QML engine is now destroyed before the objects it shows, removing shutdown warnings. Urdu Nastaliq clipped in fixed-height controls; heights now include a script allowance and small text scales up in Urdu. Latin text in a right-to-left layout aligned to the wrong edge; labels now use mirrored start alignment.

### Foxit workspace — 2026-11-01

Owner request (2026-11-01, second): copy Foxit's UI/UX as exactly as possible, with skins, and plan Foxit's features before/after 2.0. Decision [ADR-0007](../decisions/ADR-0007-foxit-workspace-parity.md); measured study and command map [FOXIT_PARITY.md](../FOXIT_PARITY.md) (14 owner screenshots at 100%, pixel-measured; Foxit user manual for workspace, skins, status bar and shortcuts). Nothing was extracted from Foxit's program folder.

- New QML: `TitleBar` (custom frame with system move/resize, Quick Access Toolbar, caption buttons), `RibbonBar` + `RibbonButton` (28 px tab row, 72 px command area, menus, collapse), `DocumentTabBar` (124 px tabs, Library first, overflow list), `Backstage` (File page with Recent Documents), `PreferencesDialog` (searchable categories, Cancel restores), `SkinsPanel`, `AboutDialog`. `ReaderTab` lost its command bar; the status bar follows Foxit's layout. `AppPreferences` stores skin, theme colour, ribbon collapsed, status-bar mode and up to 20 recent files.
- Options: `--native-frame` (system caption fallback) and `--show backstage|skins|preferences[:page]|about|ribbon-<tab>` for visual QA.
- Rendered self-review (Qt 6.4 / Xvfb, 1280 × 800): Library and reader in English light, Urdu dark; File page and Skins (English), Preferences → Languages (Arabic), View ribbon (Urdu). Found and fixed: `onAccent`/`onTitleBar` colour tokens were read as signal-handler names and rendered black (renamed `accentText`/`titleBarText`); Preferences category column swallowed the page area; the Library tab collapsed to its text width; the page box reversed "12 / 340" in right-to-left layouts.
- Known gaps: navigation rail and panels arrive with their first panel (Pages, N4.4); Snap Layouts flyout on the maximize button (native hit-test) tracked for N10; window behaviour must be checked on Windows in the review build.

### Complete plan and logo colours — 2026-11-01

- Owner: use the reference package fully and plan every Foxit feature; main colour from the logo. Inventory extracted on the owner's PC (read-only) from `data/pe-resources.json` (English menus and dialog titles), `data/modules.json` and the skin XML; nothing from the package is shipped.
- Logo palette measured from `assets/branding/atlas-reader-icon.png` (navy `#002058`–`#003880`, royal blue `#0050b0`, cyan `#00a0f8`, ice `#e0f0f8`); contrast checked (accent 7.7:1 on white, Sky 6.6:1 on `#333333`).
- Rendered check (Qt 6.4 / Xvfb): Library English light and reader Urdu dark with the new palette.

## N4.4 log

### Slice 1: Pages panel, view history, tab menu — 2026-11-02

- `core/reader/ViewHistory` (pure C++): page-relative positions that survive zoom/rotation, nearby positions collapse (2 % of a page), 100-entry cap, Forward cleared by a new jump. `ReaderViewport` records jumps (page box, first/last, Home/End, thumbnails) and exposes `goBack`/`goForward`, `canGoBack`/`canGoForward`, Alt+Left/Right, and `pageSizePoints()` for thumbnails.
- `PageThumbnailProvider` (`image://atlasthumb/…`): one background thread; each open tab's PDF is read once into memory (file never locked) and rendered at thumbnail size with the current reading colours; closing a tab frees its copy. Session sources are copied on the GUI thread whenever the reader model changes; passwords stay in memory only.
- QML: navigation rail (40 px) and `PagesPanel` (Foxit Pages panel) with resizable splitter; panel and width remembered in `preferences.ini`; F4 and View → View Setting → Navigation Panels. Status bar gains Previous/Next View. Document-tab context menu (close variants, copy path, open containing folder via Explorer `/select`).
- Known limits: each open tab holds a second in-memory copy of its PDF while the Pages panel is used (one for the page view, one for thumbnails); to be merged into one shared buffer with the N4.5 memory work.
