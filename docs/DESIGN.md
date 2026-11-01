# Atlas Reader — Design System, Accessibility and Arabic/RTL

This is the design contract. A user-facing change is done only when it meets this document in its **rendered** form, in **English, Arabic and Urdu**.

**Foxit workspace, Atlas colours (owner decision 2026-11-01, [ADR-0007](decisions/ADR-0007-foxit-workspace-parity.md)).** The workspace reproduces Foxit PDF Editor's layout, measurements, command placement and shortcuts, measured in [FOXIT_PARITY.md](FOXIT_PARITY.md). Icons are Fluent UI System Icons (MIT) at Foxit's sizes; colours, wording and name are Atlas's. Nothing is extracted from Foxit's program files.

## 1. Character

**A mature desktop PDF workspace with a quiet study-room palette.** Atlas should feel like the tool its users already trust (Foxit's ribbon workspace: same places, sizes and keys) while its colours, type and states stay calm, warm and precise. It is not a decorative consumer reader or a generic "AI app".

Atlas looks **designed** when it has:

- one warm paper-toned surface family and one ink-green accent, used sparingly;
- hierarchy built from type size, weight and space, not from boxes and color;
- everything aligned to a 4 px grid, with generous space in the Library and calm density in reader panels;
- borders before shadows; a single soft shadow only for the page and floating surfaces;
- real empty, loading and error states written in plain reader language;
- the book page as the visual hero, with interface chrome receding while you read.

Atlas never has: invented layouts where Foxit already has a familiar one, dashboards of equal cards, decorative gradients, glow, inflated headings, novelty icons, filler copy, large square grey defaults, or engineering jargon in labels.

The application mark in `assets/branding` is used for app and package identity only. Its illustrative style is not a license to decorate the working interface.

## 2. Principles

1. **Content first.** The page dominates; chrome collapses when not needed.
2. **One obvious primary action per state.** Advanced tools are discoverable, never crowding.
3. **Fast first interaction.** Show cached state instantly; the layout stays stable while loading.
4. **Keyboard parity.** Everything the pointer can do, the keyboard can do.
5. **Honest states.** Say what happened and what is safe to do next.
6. **Arabic is native.** RTL is a real layout mode, not a translated LTR screen.
7. **Undo over confirm.** Confirm only destructive or irreversible actions.
8. **Polish never hides problems.** A beautiful screen with a broken state fails review.

## 3. Structure

The full measured layout is in [FOXIT_PARITY.md §1](FOXIT_PARITY.md#1-window-anatomy-measured). Mirrored completely in Arabic and Urdu.

```text
┌ title bar 48: mark │ QAT: Open · Touch ▾ │ file name ………………… (search, N5) │ ─ □ ✕ ┐
├ ribbon tabs 28: File  Home  View  Accessibility  Help   (+ Comment N6 …)       ┤
├ ribbon 72: [Hand] │ [Zoom▾][Page Fit▾][Rotate View▾] │ …                    ⌃ ┤
├ 7 px                                                                          ┤
├ tabs 28: [Library][book A.pdf ✕][book B.pdf]   (124 px each)                ▾ ┤
│rail│ panel (F4)│                                                          │sb│
│ 40 │           │                  page canvas                             │  │
├────┴───────────┴ status 32: « ‹ [ 12 / 340 ] › »  ……  layouts − ━●━ + 125% ⛶ ┤
```

- **Title bar (custom, Windows snap kept):** app mark; Quick Access Toolbar (Open, Mouse/Touch mode; Save/Undo/Redo from N5, Print from N7; customize from N7); file name; title-bar search box from N5 (Alt+Q); minimize, maximize, close. Colourful skin tints it with the theme colour; Classic keeps it neutral.
- **Ribbon:** *File* opens the File page (backstage). Task tabs follow Foxit's order and appear only with working commands: **Home · View · Accessibility · Help** in N4.3, **Comment** in N6, others after 2.0. Commands are grouped with 1 px separators; large buttons (24 px icon over a 12 px label, ▾ for menus) and small buttons (16 px icon, 2–3 per column). Ctrl+F11 or the ⌃ button collapses the ribbon to its tab row; clicking a tab then shows it as a temporary overlay.
- **Document tabs:** the **Library** is the first, pinned tab (Foxit's *Start* tab). Book tabs are 124 px wide, filename-first with ellipsis, close ✕ on the active or hovered tab, Ctrl+Tab cycling, middle-click closes, overflow ▾ lists all tabs.
- **Navigation rail:** 40 px, icon buttons for panels (Pages N4.4, Outline/Search N4.5, Bookmarks N5, Comments N6); a panel opens beside the rail, resizable (200–480 px), F4 toggles; icons appear only when their panel works.
- **Status bar:** « ‹ page box "n / total" › » on the leading side; page-display buttons (N4.4), zoom −, slider, +, percentage menu and Full screen on the trailing side. View → View Setting can show, auto-hide or hide it.
- **File page:** left list (Open, Close, Close All, Skins, Preferences, About, Exit; Save/Print/Properties as their stages land); Open shows *Recent Documents* (from the Library's history) and *Computer* (file dialog).
- **Preferences (Ctrl+K):** window with a search box over a category list (General, Languages, Skins, Page Display, Accessibility in N4.3; more per stage) and OK / Cancel.
- **Read mode (Ctrl+H):** hides title bar, ribbon, tabs, rail and status bar; a floating mini bar appears on mouse movement or focus. **Full screen (F11):** the page fills the screen; Esc exits.

## 4. Tokens

All values live in one QML singleton, `Theme`. Screens never contain literal colors, font sizes, radii or durations. The starting palette is the one already shipped in N3/N4.1, now formalized. Contrast ratios below are measured against WCAG 2.2.

### 4.1 Color

Chrome roles follow Foxit's neutral structure (measured in FOXIT_PARITY §1) in Atlas's warm values:

| Token | Light | Dark | Use |
|---|---|---|---|
| `chrome` | `#fbfaf7` | `#22262c` | Ribbon and tab row |
| `rail` | `#f4f2ec` | `#1d2126` | Navigation rail, File page list |
| `sunken` | `#efece4` | `#1b1e23` | Gap under the ribbon, inactive document tabs |
| `titleBar` | `#f4f2ec` (Classic) or theme colour (Colourful) | `#16191d` | Title bar |
| `hover` | `#e9e6dd` | `#2f343c` | Hover, pressed, group separators |

Theme colours for the Colourful skin: Atlas Green `#28633a` (default), Indigo `#3f4a9a`, Teal `#16656b`, Plum `#6b3a6b`, Amber `#8a5a12`, Graphite `#3c4148`. Each keeps white title-bar text at ≥ 7:1.


| Token | Light | Dark | Use | Contrast |
|---|---|---|---|---|
| `canvas` | `#f5f3ed` | `#17191d` | App background | — |
| `panel` | `#fffdf8` | `#20242a` | Sidebars, sheets | — |
| `raised` | `#ffffff` | `#292e36` | Cards, menus, fields | — |
| `ink` | `#20231f` | `#f4f2eb` | Primary text | 14.3 / 15.7 on canvas |
| `muted` | `#667064` | `#aeb5bf` | Secondary text | 4.65 / 8.5 on canvas (AA) |
| `line` | `#d9ddd4` | `#3a414b` | Decorative dividers only | 1.2 / 1.7 (not for controls) |
| `controlBorder` | `#7d867a` | `#757e8a` | Field and control outlines | ≥ 3.4 / 3.8 (WCAG 1.4.11) |
| `accent` | `#28633a` | `#9fc8a7` | Primary action, selection, focus | 6.45 / 9.5 on canvas |
| `accentText` | `#ffffff` | `#17191d` | Text on accent | 7.2 / 9.5 |
| `accentSoft` | `#e3f0e5` | `#243d2c` | Selected rows, info banners | accent text 6.1 / 6.4 |
| `danger` / `dangerSoft` | `#7a201c` / `#fff0ee` | `#ffd7d3` / `#4a2525` | Errors | 9.2 / 10.1 |
| `warning` | `#87550b` | `#efc27e` | Caution text and icons | 6.3 / 8.3 |
| `readerBackdrop` | `#e9e6de` | `#0f1113` | Around pages | — |
| `focusRing` | = `accent` | = `accent` | 2 px ring with a 2 px gap | ≥ 3:1 |

The storage states (Embedded, Local only, Pending, Conflict, Signed, Read-only, Offline, Locked) each get **an icon plus text**. Color only supports them.

**High contrast:** when Windows contrast themes are active, use system colors (`palette`) for every token and drop shadows and soft fills. System theme beats the brand palette.

**Reading themes (N4.5):** Paper (default), Sepia, Dark and an optional *Invert page colors* that changes rendering only and never the PDF. UI theme and page theme are set separately.

### 4.2 Typography

| Role | Size / line height | Weight | Notes |
|---|---|---|---|
| `display` | 28 / 36 | Semibold | Library title only |
| `title` | 20 / 28 | Semibold | Sheet and dialog titles |
| `heading` | 16 / 24 | Semibold | Section headings |
| `body` | 14 / 21 | Regular | Default UI text |
| `bodyStrong` | 14 / 21 | Semibold | Names in lists |
| `caption` | 12 / 18 | Regular | Metadata, hints (never smaller) |
| `label` | 12 / 16 | Semibold | Small caps-free section labels |

- **Latin UI:** Segoe UI Variable (Windows system font, so nothing is bundled).
- **Arabic UI:** Segoe UI's Arabic glyphs as the baseline. Noto Sans Arabic UI (OFL) is a candidate if the N4.5 rendered comparison shows better joining and diacritic clarity at 12–14 px. Bundling needs the [dependency gate](DEPENDENCIES.md).
- **Urdu UI and Urdu content:** **Noto Nastaliq Urdu** (bundled, OFL, the same pinned file as the N2 evidence). Nastaliq needs room: Urdu body text uses 15 px on 30 px line height, controls grow to fit (never clip), and labels are kept short.
- **Arabic content** (bookmark titles, notes, document titles): **Noto Naskh Arabic** (bundled, OFL) at +1 px; Arabic UI chrome uses Segoe UI.
- **Any other script** in user content falls back to Windows system fonts automatically (Segoe UI, Nirmala UI for Devanagari, Microsoft YaHei/Yu Gothic/Malgun Gothic for CJK, Segoe UI Emoji), so nothing renders as empty boxes.
- **Arabic rules:** line height +15–20% (body 14/24); no letter-spacing ever (it breaks joining); no faux italics; avoid semibold below 13 px; keep diacritics clear of underlines and focus rings.
- Numbers in page labels and counts use tabular figures where available.

### 4.3 Space, shape, depth, motion

- **Spacing (4 px grid):** `xs 4 · sm 8 · md 12 · lg 16 · xl 24 · xxl 32 · xxxl 48`.
- **Radii:** `small 4` (chips, badges) · `control 8` (buttons, fields, rows) · `surface 12` (sheets, menus, dialogs). The page itself has a 0–2 px radius.
- **Depth:** level 0 is flat with `line`; level 1 (menus, popovers) uses a border and a 0 2 8 shadow at 12% black; the page uses a 0 1 3 shadow at 15% black. No other shadows.
- **Sizes (Foxit-measured):** title bar 48, ribbon tab row 28, ribbon command area 72, document tab bar 28 (tabs 124 wide), rail 40, status bar 32; large ribbon button min 42 wide with a 24 px icon, small ribbon button 24 × 24 with a 16 px icon, Quick Access buttons 32 wide with 16 px icons. Minimum pointer target 24 px (WCAG 2.2); Touch mode adds 8 px to every target and bar. Library controls 32 px high.
- **Motion:** `fast 120 ms` (hover, press), `normal 180 ms` (panels, menus), `slow 240 ms` (sheets). Ease-out to enter, ease-in to exit; always interruptible. With reduced motion, durations are 0 and nothing scales or slides. Large page surfaces never animate in a way that costs frame time.

## 5. Components

Each component ships with default, hover, pressed, focused, disabled and (where relevant) selected, error and loading states, in both directions and themes.

| Component | Variants and rules |
|---|---|
| `AtlasButton` | Primary (accent fill), Secondary (border), Quiet (text), Danger. One primary per view. |
| `AtlasIconButton` | Always has a tooltip and an accessible name; 32 px target. |
| `AtlasTextField` | Label above, never placeholder-as-label; `controlBorder`; inline error with icon and text. |
| `AtlasTabStrip` | Filename-first titles with ellipsis in the middle; close button inside the tab; `Ctrl+Tab` order; selected tab kept visible. |
| `AtlasSidePanel` | Collapsible, resizable, remembers width; drawer when narrow. |
| `AtlasTree` | Outline and bookmarks: arrow-key semantics, type-ahead, breadcrumbs, RTL indentation, accessible expanded/selected states. |
| `AtlasBanner` | Info, warning, error; icon + one sentence + at most two actions. |
| `AtlasEmptyState` | One explanation + one primary action (for example, *No folders yet → Add folder*). |
| `AtlasDialog` / `AtlasSheet` | Only for destructive or preview decisions; buttons follow platform order. |
| `AtlasStateBadge` | Storage and availability states: icon + text + tooltip with details. |
| `AtlasProgress` | Inline and calm; appears only when work is perceptible (over about 400 ms). |
| `AtlasToast` / announcer | Non-modal results, also announced to Narrator without moving focus. |

## 6. Words and states

Write the way a helpful librarian speaks: short, specific and calm. Never "Error 32" or a bare "Save failed" when Atlas knows the cause.

> **This PDF is read-only. Your bookmark is saved in Atlas.** You can add it to the PDF later, or save a copy now.

- **Loading:** show results as they arrive; never block on 100%.
- **Empty:** one sentence and one action.
- **Restricted or error:** keep everything that still works usable. A read-only PDF is not a broken book.
- **Recovery choices**, only those that are safe: *Retry · Keep in Atlas · Save a copy · Locate file · Export changes · Discard changes*.
- Preview is required before: destructive import or replace, reconciliation conflicts, ambiguous restore or migration matches, writing metadata into a PDF, and working-copy decisions on signed files.
- Password prompts distinguish *open the document* from *allow changes*.

## 7. Keyboard

Foxit's default keys for every command Atlas has ([FOXIT_PARITY §4](FOXIT_PARITY.md#4-keyboard)), so muscle memory carries over.

| Action | Keys |
|---|---|
| Open PDF | `Ctrl+O` (in the Library, *Add folder* is on its button and `Ctrl+Shift+O`) |
| Close tab / all tabs / exit | `Ctrl+W` or `Ctrl+F4` / `Ctrl+Shift+W` / `Ctrl+Q` |
| Preferences | `Ctrl+K` |
| Collapse or pin the ribbon | `Ctrl+F11` |
| Next / previous tab | `Ctrl+Tab` / `Ctrl+Shift+Tab` |
| Title-bar search / Command Center | `Alt+Q` (N5) |
| Find in the Library | `Ctrl+F` |
| Find in document / next / previous | `Ctrl+F` / `F3` / `Shift+F3` (N4.5) |
| Go to page | `Ctrl+G` (focuses the page box) |
| Next / previous page | `Ctrl+Page Down` / `Ctrl+Page Up`, also `→` `←` when the page fits |
| First / last page | `Ctrl+Home` / `Ctrl+End` |
| Scroll a screen | `Page Down`/`Space`, `Page Up`/`Shift+Space` |
| Zoom in / out | `Ctrl++` (`Ctrl+=`) / `Ctrl+-`, `Ctrl+wheel` at the pointer |
| Actual size / Fit width / Fit page | `Ctrl+1` / `Ctrl+2` / `Ctrl+0` |
| Rotate view clockwise / counter-clockwise | `Ctrl+Shift++` / `Ctrl+Shift+-` |
| Show/hide navigation panel | `F4` |
| Read mode / Full screen | `Ctrl+H` / `F11` (`Esc` exits either) |
| Back / Forward (history) | `Alt+←` / `Alt+→` (N4.4; temporal, not mirrored in RTL) |
| Add bookmark | `Ctrl+B` (N5) |
| Save portable changes | `Ctrl+S` (N5+) |
| Print | `Ctrl+P` (N7) |
| Undo / redo | `Ctrl+Z` / `Ctrl+Y` or `Ctrl+Shift+Z` |
| Rename / delete / activate | `F2` / `Delete` / `Enter` |
| Context menu | `Shift+F10` or the Menu key |

Shortcuts appear in tooltips and menus, and their labels are localized; the keys themselves are the same in every language. Trees use standard arrow semantics (mirrored in RTL).

## 8. Accessibility contract

Every interactive element has an accessible **role, name and state**; a logical focus order; a visible focus ring; keyboard activation; and no pointer-only action. In addition:

- The reader canvas exposes page, position, zoom and actions, and from N4.4 the document's text and reading order (tagged versus untagged versus image-only is stated honestly).
- Announce, without stealing focus: save results, local-only fallback, search result counts, page jumps, import/export completion, and offline/reconnected states.
- UI text scaling and PDF zoom are independent.
- Text wraps instead of clipping at 200%; check 125% and 150% as well.
- Never use color alone; honor reduced motion; support Windows contrast themes.
- **Reading colours** (Foxit's "replace document colours" concept, rendering only, never written to the PDF): Paper (default), Sepia, Night (light text on dark), High contrast (black/white or white/black) and *Follow Windows contrast theme*; a custom page/text colour pair arrives with Settings (N7).
- **Touch mode:** one switch enlarges every target to 40 px and spacing to match.
- **Caret browsing and system selection colour** options for keyboard and magnifier users (N4.5).
- Tools: Accessibility Insights for diagnostics. **Narrator by hand is still required**, because automated checks don't prove a usable flow.

## 9. Languages: English, Arabic, Urdu and any-script content

- **Interface languages in 2.0: English (default), Arabic, Urdu.** Switched live from the title bar, remembered between runs, overridable with `--language en|ar|ur`. Every string exists in all three; no screen ships with an English leftover in Arabic or Urdu.
- Translations are written for meaning and tone, not word-for-word: short, polite, plain reader language. Arabic uses Modern Standard Arabic; Urdu uses everyday standard Urdu with common technical loanwords where Urdu readers expect them (for example پی ڈی ایف).
- Arabic and Urdu mirror the structure with `LayoutMirroring.enabled` and `childrenInherit` at the window root.
- **User content is any language.** Every input accepts any script via Windows keyboards/IMEs; text direction of each field follows its content (first strong character), not the UI language.
- User content is never force-reversed. Filenames, paths and mixed segments get bidi isolation.
- Directional icons (back, forward, next page in an LTR book) mirror with UI direction. Universal icons (search, close, play) do not. Page progression follows the **book's** reading direction, not the UI language.
- Tree indentation, breadcrumbs and chevrons read naturally in RTL.
- Arabic, English, Urdu, punctuation, brackets, Arabic-Indic and Western digits, and diacritics are always tested together.
- Search normalization (alef forms, taa marbuta, diacritics, tatweel) is a documented policy. Stored text is never altered to make search easier.

## 10. Design review

Before a UI change is called done, the author captures and inspects screenshots for:

| Axis | Values |
|---|---|
| Language | English, Arabic |
| Theme | Light, Dark (+ Windows high contrast at N4.5, N9) |
| Scale | 100%, 200% |
| Width | Wide (≥ 1280), narrow (≈ 760) |
| State | Populated, empty, loading, error/restricted, long names |

Then answer every question:

1. Is the primary task obvious within two seconds?
2. Is the page still the dominant surface?
3. Are alignment, spacing and type on the token scale, with no stray values?
4. Can it be completed by keyboard alone, with focus always visible?
5. Does Narrator announce useful names, roles and states?
6. Is it correct and comfortable in Arabic?
7. Does it fit at 200% without clipping?
8. Are local, portable and error states understandable, and is every error recoverable?
9. Is advanced functionality hidden until needed, not removed?
10. Does it add any synchronous work or frame instability?
11. Would an experienced product designer call it finished: calm, consistent, nothing generic?

Screenshots and answers go into the stage evidence ledger. Raw screenshots stay in ignored `artifacts/`.

## 11. Implementation in Qt Quick

- `qml/Theme.qml` (`pragma Singleton`) holds every token in §4 and switches on dark mode, Windows contrast and touch mode.
- `qml/I18n.qml` (`pragma Singleton`) exposes `language`, `rtl`, `t(key, args…)` and font families per language; strings live in `qml/i18n/Strings.js` as one table per language. Bindings that call `I18n.t()` re-evaluate when the language changes, so switching is instant with no restart ([ADR-0006](decisions/ADR-0006-interface-languages.md)). `tools/i18n/check-strings.py` (CI) fails if any key is missing, empty or has mismatched `%1` placeholders in any language.
- Fonts: Noto Nastaliq Urdu and Noto Naskh Arabic ship in `assets/fonts/` (OFL, pinned SHA-256) and load at startup.
- Icons: one outline family (Fluent UI System Icons, MIT, pinned) in `assets/icons/`, tinted through the control's `icon.color`; directional icons mirror in RTL.
- Components and views live in `qml/`; `Main.qml` only composes views. No literal colours, sizes or strings in views.
- Respect `Qt.styleHints` for reduced motion and Windows accessibility settings.

## 12. Touch and pen

Windows is mouse and keyboard first, but:

- Touch mode enlarges targets to 40 px.
- Panning never creates ink. Writing mode has a clear exit that never loses unsaved work.
- Stylus input goes to a low-latency live layer (see [ARCHITECTURE.md](ARCHITECTURE.md)). Barrel and eraser buttons and palm rejection are mapped only after hardware tests.
