# Atlas Reader — Design System, Accessibility and Arabic/RTL

This is the design contract. A user-facing change is done only when it meets this document in its **rendered** form, in **English, Arabic and Urdu**.

**Foxit concepts, Atlas design (owner decision 2026-11-01).** Atlas adopts the main interaction and accessibility *concepts* of Foxit PDF Editor (studied from the installed app and its manual) so Foxit users feel at home: see the concept table in [PLAN.md §7](../PLAN.md#7-competitive-references). Atlas never copies Foxit's artwork, icons, branding, colours, wording or files; the look below is Atlas's own.

## 1. Character

**A quiet study room.** Atlas should feel like a serious research instrument made by an experienced product team: calm, warm, precise and durable. It is not an office-suite toolbar wall, a decorative consumer reader, or a generic "AI app".

Atlas looks **designed** when it has:

- one warm paper-toned surface family and one ink-green accent, used sparingly;
- hierarchy built from type size, weight and space, not from boxes and color;
- everything aligned to a 4 px grid, with generous space in the Library and calm density in reader panels;
- borders before shadows; a single soft shadow only for the page and floating surfaces;
- real empty, loading and error states written in plain reader language;
- the book page as the visual hero, with interface chrome receding while you read.

Atlas never has: dashboards of equal cards, decorative gradients, glow, inflated headings, novelty icons, filler copy, large square grey defaults, or engineering jargon in labels.

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

**Shell destinations:** Library · Recents · Favorites · Folders · Bookmarks (N5) · Settings (N7).

**Reader workspace** (Foxit concepts in Atlas styling; mirrored automatically in Arabic and Urdu):

```text
┌ title bar: mark · Library/location · ………… · Command Center (Ctrl+K) · language · theme · Open ┐
├ tab bar: [book A ×] [book B ×] [+]                                                         ┤
├ command bar: File | Navigate | Zoom | View   (one row; groups collapse into "More" when narrow) ┤
│ rail │ panel (F4)  │                                                                   │
│ ▤    │ Outline     │                  page canvas (dominant)                            │
│ ▦    │ Pages       │          gutters 24 px · soft page shadow · reading colours        │
│ 🔖   │ Bookmarks   │                                                                   │
│ 🔍   │ Search      │                                                                   │
├──────┴─────────────┴ status bar: |◀ ◀ [ 12 ] / 340 ▶ ▶| · layout · ──●── 125% · fit ──────┤
```

- **Title bar:** app mark and current place (Library or book name); the Command Center box (Ctrl+K, Alt+Q also works) for books, bookmarks and commands; language menu (English · العربية · اردو); theme; Open.
- **Tab bar:** filename-first titles, close on hover/focus, `+` to open, Ctrl+Tab cycling, middle-click closes.
- **Command bar:** a single calm row of grouped commands, not a ribbon wall. *File* (Open, Close) · *Navigate* (first/previous/next/last) · *Zoom* (out, %, in, Fit width, Fit page, Actual size) · *View* (Rotate, Read mode, Full screen, Reading colours). Labels show beside icons when space allows; below about 900 px only icons with tooltips remain; overflow goes into **More (⋯)**. Later stages add *Bookmark* (N5) and *Annotate* (N6) groups.
- **Left rail + panel:** icon rail always visible; clicking an icon opens its panel (Outline, Pages, Bookmarks, Annotations, Search); clicking again or F4 collapses. The panel is resizable (200–480 px) and remembers its width. In narrow windows it overlays the page as a drawer with the same order and keys. Icons appear only when their panel works.
- **Status bar:** page navigation with a page box that accepts numbers and (from N4.4) labels, layout buttons (single/continuous/facing from N4.4), zoom slider with percentage, fit buttons. It can auto-hide in Read mode.
- **Read mode (Ctrl+H):** hides title, command and status bars and the panel; a small floating bar (page, zoom, exit) appears near the bottom on mouse movement or focus. **Full screen (F11):** the page fills the screen; Esc exits; both restore focus where it was.
- Progressive disclosure: details, export/import, reconciliation, save copy, advanced print and diagnostics live in menus and sheets, never in permanent toolbars.

## 4. Tokens

All values live in one QML singleton, `Theme`. Screens never contain literal colors, font sizes, radii or durations. The starting palette is the one already shipped in N3/N4.1, now formalized. Contrast ratios below are measured against WCAG 2.2.

### 4.1 Color

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
| `onAccent` | `#ffffff` | `#17191d` | Text on accent | 7.2 / 9.5 |
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
- **Sizes:** minimum pointer target 32×32 px (WCAG 2.2 target size is 24 px); touch mode 40×40 px; icons 16 px (dense) or 20 px; standard control height 32 px, compact 28 px.
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

Foxit-compatible where a Foxit equivalent exists, so muscle memory carries over.

| Action | Keys |
|---|---|
| Open PDF | `Ctrl+O` (in the Library, *Add folder* is on its button and `Ctrl+Shift+A`) |
| Close tab / all tabs | `Ctrl+W` or `Ctrl+F4` / `Ctrl+Shift+W` |
| Next / previous tab | `Ctrl+Tab` / `Ctrl+Shift+Tab` |
| Command Center | `Ctrl+K` or `Alt+Q` (N5) |
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
