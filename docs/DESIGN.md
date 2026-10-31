# Atlas Reader — Design System, Accessibility and Arabic/RTL

This is the design contract. A user-facing change is done only when it meets this document in its **rendered** form. Competitor products are studied for capability gaps, never copied: no Foxit/Acrobat ribbon, layout, colors, icons or artwork.

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

**Reader workspace:**

```text
┌ tab strip ────────────────────────────────────────────────┐
│ ◧ panel │  page toolbar: page · zoom/fit · search · more  │
│ Outline │                                                 │
│ Pages   │          page canvas (dominant)                 │
│ Bookmk  │     gutters 24–32 px · soft page shadow         │
│ Annot.  │                                                 │
└─────────┴─────────────── status (only when needed) ───────┘
```

- The navigation panel order is Outline → Pages → Bookmarks → Annotations. It keeps the current location in sync and has its own search. In narrow windows it becomes a drawer with the same order and keys.
- The toolbar shows only frequent actions: navigation, zoom/fit, search, bookmark (N5), write mode (N6) and reader settings. Details, export/import, reconciliation, save copy, advanced print and diagnostics live in menus and sheets.
- Status for dirty, pending or local-only work is visible but never dominates reading.

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
- **Arabic/Urdu content fixtures:** Noto Naskh Arabic and Noto Nastaliq Urdu, already pinned for N2 evidence.
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

| Action | Keys |
|---|---|
| Open PDF | `Ctrl+O` (final in N4.5; the Library's Add folder moves to `Ctrl+Shift+A`). `Ctrl+Shift+O` stays as an alias. |
| Command Center | `Ctrl+K` (N5) |
| Find in document | `Ctrl+F`, `F3` / `Shift+F3` |
| Go to page | `Ctrl+G` |
| Zoom in / out / fit width / fit page / actual size | `Ctrl+=` / `Ctrl+-` / `Ctrl+1` / `Ctrl+2` / `Ctrl+0` |
| Back / Forward (history) | `Alt+←` / `Alt+→` (mirrored meaning in RTL is **not** applied: history is temporal) |
| Toggle navigation panel | `Ctrl+Shift+L` |
| Focus mode | `F11`; `Esc` exits and restores focus |
| Add bookmark | `Ctrl+B` (N5) |
| Save / commit portable changes | `Ctrl+S` (N5+) |
| Print | `Ctrl+P` (N7) |
| Close tab / next / previous | `Ctrl+W` / `Ctrl+Tab` / `Ctrl+Shift+Tab` |
| Undo / redo | `Ctrl+Z` / `Ctrl+Shift+Z` |
| Rename / delete / activate | `F2` / `Delete` / `Enter` |
| Context menu | `Shift+F10` or the Menu key |

Shortcuts appear in tooltips and menus, and their labels are localized. The keys themselves don't change by language. Trees use standard arrow semantics.

## 8. Accessibility contract

Every interactive element has an accessible **role, name and state**; a logical focus order; a visible focus ring; keyboard activation; and no pointer-only action. In addition:

- The reader canvas exposes page, position, zoom and actions, and from N4.4 the document's text and reading order (tagged versus untagged versus image-only is stated honestly).
- Announce, without stealing focus: save results, local-only fallback, search result counts, page jumps, import/export completion, and offline/reconnected states.
- UI text scaling and PDF zoom are independent.
- Text wraps instead of clipping at 200%; check 125% and 150% as well.
- Never use color alone; honor reduced motion; support Windows contrast themes.
- Tools: Accessibility Insights for diagnostics. **Narrator by hand is still required**, because automated checks don't prove a usable flow.

## 9. Arabic, Urdu and RTL

- Arabic UI mirrors the structure with `LayoutMirroring.enabled` and `childrenInherit` at the window root.
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

- `qml/Atlas/Theme.qml` (`pragma Singleton`) holds every token in §4, switching on dark mode and `Qt.styleHints.colorScheme`; `qml/Atlas/qmldir` registers it and the components.
- Components live in `qml/Atlas/` and screens in `qml/views/`. `Main.qml` only composes views.
- Strings use `qsTr()`; `i18n/atlas_en.ts` and `i18n/atlas_ar.ts` are compiled by `qt_add_translations`. Remove inline `arabic ? … : …` strings.
- Icons: one consistent outline set at 16/20 px. The candidate is Fluent UI System Icons (MIT), loaded as SVG through Qt SVG, subject to the dependency gate. Never mix icon families.
- CI later fails on literal `#rrggbb` colors in `qml/views/` (allow-list: `Theme.qml`).
- Respect `Qt.styleHints` for reduced motion and Windows accessibility settings.

## 12. Touch and pen

Windows is mouse and keyboard first, but:

- Touch mode enlarges targets to 40 px.
- Panning never creates ink. Writing mode has a clear exit that never loses unsaved work.
- Stylus input goes to a low-latency live layer (see [ARCHITECTURE.md](ARCHITECTURE.md)). Barrel and eraser buttons and palm rejection are mapped only after hardware tests.
