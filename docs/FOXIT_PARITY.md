# Atlas Reader — Foxit Workspace Parity

<!-- Living reference. Decision: ADR-0007. -->

**Owner decision (2026-11-01):** Atlas copies the *layout, measurements, command placement, interaction model and keyboard shortcuts* of Foxit PDF Editor (Windows, ribbon mode, version installed on the owner's PC) as closely as possible, so the product feels like a mature desktop PDF tool instead of a generic "AI-looking" app. Foxit features are added in Atlas's own stages: what a reader and research tool needs before 2.0, the rest after 2.0 ([ADR-0007](decisions/ADR-0007-foxit-workspace-parity.md)).

**What is copied:** structure, sizes, spacing, order of tabs, groups and commands, labels (translated), menus, panels, status bar, File page, Preferences window, skins model, Touch mode, shortcuts and single-key accelerators.

**What is not copied:** Foxit's icon artwork, logos, skin images, colours used as brand (purple), text of their manual, or any file from `C:\Program Files\Foxit Software`. Those are Foxit's copyrighted assets and the licence forbids extracting them. Atlas uses Fluent UI System Icons (MIT, thin-line, same 16/20/24 px grid), its own colours and its own name.

**Sources:** 14 owner screenshots of Foxit PDF Editor at 100% scale (1919 × 1044 window), measured with pixel analysis on 2026-11-01; Foxit PDF Editor User Manual (workspace, skins, Touch mode, navigation pane, status bar, keyboard shortcuts appendix). Measurements are in device-independent pixels at 100%.

---

## 1. Window anatomy (measured)

```
y   0 ┌──────────────────────────────────────────────────────────────────────┐
      │ Title bar 48 px: app mark · Quick Access Toolbar · file name ·        │
      │                  … · search · notifications · ─ □ ✕                  │
y  49 ├ line ───────────────────────────────────────────────────────────────┤
y  50 │ Ribbon tab row 28 px: File  Home  Convert  Edit  …  Help              │
y  78 │ Ribbon command area 72 px: [Hand][Select]│[Zoom][Fit]…│…              │
y 150 │ 4 px                                                              ⌃  │
y 154 ├ line ───────────────────────────────────────────────────────────────┤
y 155 │ 7 px gap                                                             │
y 162 ├ Tab bar 28 px: [Start][book.pdf][book 2.pdf ✕] … fixed 124 px tabs ▾ ┤
y 190 ├──┬──┬─────────────────────────────────────────────────────────────┬──┤
      │R │▸ │ Document pane                                              │sb│
      │a │  │                                                            │  │
      │i │  │                                                            │  │
      │l │  │                                                            │  │
      ├──┴──┴─────────────────────────────────────────────────────────────┴──┤
      │ Status bar: « ‹ [ 76 / 459 ] › » · view   …   layouts − ━━●━ + 100% ⛶│
      └──────────────────────────────────────────────────────────────────────┘
```

| Part | Measurement (100% scale) | Atlas token |
|---|---|---|
| Title bar | 48 px high, 1 px bottom line | `Theme.titleBarHeight` 48 |
| App mark | 16 px at x 12, vertically centred | |
| Quick Access Toolbar | 16 px icons, 32 px pitch, starts at x 53 after a 1 px separator at x 40 | `Theme.qatButton` 32 |
| File name | 12 px text at x ≈ 358, after a 16 px document icon | |
| Window buttons | 16 px glyphs on a 42 px pitch (min, max, close), right-aligned | `Theme.captionButton` 42 |
| Ribbon tab row | 28 px; labels 14 px; 16 px padding each side; selected = accent text + 3 px underline the width of the text, 2 px below the text | `Theme.ribbonTabHeight` 28 |
| Ribbon command area | 72 px; large button: icon 24 px with 6 px top padding, label 12 px one or two lines, ▾ for menus; min width 42; 8 px side padding; pressed/hover fill, radius 2 | `Theme.ribbonHeight` 72 |
| Small ribbon button | 24 × 24 with 16 px icon, laid out 2–3 per column (Comment markup, View page layouts, Form fields) | |
| Group separator | 1 px line, full command-area height, 4 px margin each side | |
| Ribbon bottom | 4 px padding, 1 px line; collapse ⌃ at right (Ctrl+F11) | |
| Gap above tab bar | 7 px | |
| Document tab bar | 28 px; tabs fixed 124 px + 1 px divider; 12 px label with 8 px left padding; active tab uses the page colour and shows ✕; overflow ▾ at right | `Theme.docTabWidth` 124 |
| Navigation rail | 40 px; 20 px icons; first icon 28 px below the tab bar; 45 px pitch; collapse handle 14 px when panels hidden; panel opens to the right (default 240 px, resizable) | `Theme.railWidth` 40 |
| Scroll bar | 15 px system style | |
| Status bar | 32 px; page navigation left, view and zoom right | `Theme.statusBarHeight` 32 |
| File page (backstage) | left list 210 px, items 31 px, selected item filled with accent (10 px inset, radius 2); second column 300 px; recent-file rows 54 px (16 px file icon, 14 px name, 12 px path) | |
| Preferences window | ~770 × 500, category list 160 px with a search box on top, OK / Cancel bottom right | |

Neutral light colours measured (Atlas reuses the *structure*, with its own warm values in [DESIGN.md §4](DESIGN.md)):

| Role | Foxit light | Atlas light |
|---|---|---|
| Ribbon surface | `#FBFBFD` | `Theme.chrome` |
| Rails, File page list | `#F5F6FA` | `Theme.rail` |
| Gaps, inactive tabs | `#F0F1F5` | `Theme.sunken` |
| Lines | `#C7CCD9` | `Theme.line` |
| Hover / pressed / separators | `#E6E9F0` | `Theme.hover` |
| Title bar (Colourful skin) | brand purple | Atlas green `#28633A` or the chosen theme colour |

## 2. Skins

Foxit: **File → Skins**, mode Light / Dark / Use system setting; in Light mode a *Colourful* option tints the title bar with a theme colour. Atlas copies this model:

- **Mode:** Light, Dark, System.
- **Light theme:** *Classic* (neutral title bar) or *Colourful* (title bar in the theme colour). Theme colours: Atlas Green (default), Indigo, Teal, Plum, Amber, Graphite. Not Foxit purple, not blue as default.
- **Touch mode** button on the Quick Access Toolbar: larger targets in ribbon and toolbar (+8 px).
- Reading colours (Paper, Sepia, Night, High contrast) stay in **Accessibility → Reading Option** and View, like Foxit's "Replace document colours".

## 3. Command inventory and Atlas placement

Legend: **Now** = in the N4.3 workspace revamp · **N4.x / N5–N9** = Atlas stage before 2.0 · **2.x** = after 2.0 ([AFTER_2_0.md](AFTER_2_0.md)) · **No** = not planned (reason). A command only appears in Atlas when it works; until then its group is absent, never greyed out as a promise.

### Title bar and Quick Access Toolbar

| Foxit | Atlas |
|---|---|
| App menu, Open, Save, Print, Create, Undo, Redo, Hand/Select, Customize ▾ | Now: Open, Touch mode. N5: Save, Undo, Redo. N7: Print. N7: Customize (add/remove, above/below ribbon) |
| File name | Now |
| Search box (commands, text, help; Alt+Q) | N5 Command Center (commands, books, bookmarks); Alt+Q |
| Notifications | No (no accounts or cloud) |
| Min / max / close, own title bar | Now (custom title bar with Windows snap) |

### File (backstage)

| Foxit | Atlas |
|---|---|
| Open → Recent Documents, Computer | Now (Recent from Library history; Computer = file dialog) |
| Create | 2.x (Track C) |
| Properties (Ctrl+D) | N7 details panel |
| Save (Ctrl+S), Save As (Ctrl+Shift+S), Revert | N5 safe save |
| Print (Ctrl+P), Batch Print | N7 print; batch print 2.x |
| Share | No (cloud) |
| Export | N5 bookmarks export; N6 annotations export |
| Close (Ctrl+W), Close all (Ctrl+Shift+W), Exit (Ctrl+Q) | Now |
| PDF Optimizer, Index, Action Wizard | 2.x candidates (Index ≈ Atlas Library search already) |
| Skins | Now |
| Preferences (Ctrl+K) | Now: General, Languages, Skins, Page Display, Accessibility, Reading, History; more categories as features land |

### Home

| Group | Foxit commands | Atlas |
|---|---|---|
| Tools | Hand (Alt+3), Select ▾ (Text and Image Alt+6, Annotation), SnapShot (Alt+7), Clipboard ▾, Bookmark | Hand Now; Select text N4.5; Select annotation N6; SnapShot N4.5; Clipboard ▾ N4.5; Bookmark N5 |
| View | Zoom ▾ (In, Out, Marquee, Loupe, Magnifier, Zoom to Ctrl+M), Page Fit Option ▾ (Actual Ctrl+1, Fit Page Ctrl+0, Fit Width Ctrl+2, Fit Visible Ctrl+3), Reflow (Ctrl+4), Rotate View ▾ | Zoom, Page Fit, Rotate View Now; Fit Visible, Zoom to, Marquee zoom N4.4; Loupe/Magnifier 2.x; Reflow 2.x (C18) |
| Edit | Edit Text, Edit Object ▾ | 2.x (E2) |
| Comment | Typewriter, Highlight | N6 |
| Pages | Rotate Pages ▾, Insert ▾ | 2.x (E1) |
| Scan / OCR | From Scanner ▾, Quick Recognition | 2.x candidate (offline OCR with Arabic/Urdu; moved from "never" to candidate) |
| Sign | Fill & Sign | 2.x (E5) |

### Convert

| Foxit | Atlas |
|---|---|
| From Files / Scanner / Clipboard / Web Page, Form, PDF Portfolio, Make Binder, Combine Files, Blank, From Template | 2.x (Combine, Blank, From images); Web page, Portfolio, Binder: No |
| Export All Images, To Image | 2.x (render-based, offline) |
| To MS Office, To HTML, To Other | 2.x candidate: plain text / Markdown only; Office: No |
| Recognize Text, Quick Recognition, Suspect Results | 2.x candidate (offline OCR) |
| Preflight | No |

### Edit

| Foxit | Atlas |
|---|---|
| Edit Text, Edit Object, Link & Join Text, Check Spelling, Search & Replace, Add Text/Images/Shapes, Reflow Editing, Article Box | 2.x (E2) |
| Web Links ▾, Link | Following links N4.5; creating links 2.x |
| Bookmark (Ctrl+B), Auto Create Bookmarks, Cross Reference | Bookmark N5; Auto Create Bookmarks (from outline/headings) N5 candidate; Cross Reference 2.x |
| File Attachment, Image Annotation, Audio & Video, Add 3D | Attachments list (read-only, save copy) 2.x; multimedia/3D: No |

### Organize

| Foxit | Atlas |
|---|---|
| Insert, Delete, Extract, Reverse, Rearrange, Move, Duplicate, Replace, Split, Swap, Interleaving, Rotate/Crop/Resize Pages, Flatten | 2.x (E1), always to a new file by default |
| Watermark, Background, Header & Footer, Bates Numbering, Format Page Numbers | 2.x candidates; Format Page Numbers (page labels) N4.4 reads them, editing 2.x |

### Comment

| Group | Foxit | Atlas |
|---|---|---|
| Text markup | Highlight (Shift+H), Underline (Ctrl+U), Squiggly (Shift+U), Strikeout (Shift+S), Replace Text, Insert Text (Shift+I) | N6 (highlight, underline, strikeout; squiggly, replace/insert text as display + 2.x authoring) |
| Pin | Note (Ctrl+Shift+N), File | Note N6; File 2.x |
| Typewriter | Typewriter (Shift+T), Textbox (Shift+E), Callout (Shift+C) | N6 typed notes; callout 2.x |
| Drawing | Drawing ▾ (line, arrow, rectangle, oval, polygon, cloud), Pencil (Shift+P), Eraser | N6 |
| Area | Area Highlight, Search & Highlight | N6 area highlight; search & highlight 2.x |
| Measure | Measure ▾, Accounting Calculator | No (CAD/accounting) |
| Stamp | Stamp ▾, Custom Stamp ▾ | 2.x candidate |
| Symbols | Symbols, Numbers, Letters | 2.x |
| Manage | Summarize Comments, Import, Export (Ctrl+L), FDF via Email, Manage Comments (show/hide Ctrl+8 / Ctrl+Shift+8), Keep Tool Selected | N6: export, show/hide, keep tool selected; summarize 2.x; email: No |

### View

| Group | Foxit | Atlas |
|---|---|---|
| Modes | Read Mode (Ctrl+H), Reflow, Reverse View, Visual Mode ▾, Text Viewer (Ctrl+6) | Read Mode Now; Reverse View (right-to-left page order) N4.4 (C03); Visual Mode = reading colours Now; Reflow, Text Viewer 2.x |
| Rotate | Rotate View ▾ (Ctrl+Shift+Plus/Minus) | Now |
| Ruler | Toggle Ruler (Ctrl+R), Line Weights (Ctrl+5) | 2.x |
| Page display | Single Page, Continuous, Facing, Continuous Facing, Separate Cover Page, Split ▾ | N4.4 (split 2.x, C17) |
| Scrolling | AutoScroll (Ctrl+Shift+H, Esc stops) | N4.4 |
| Transitions | Page Transitions | No |
| Assistant, Read | AI Assistant, Read aloud (Ctrl+Shift+V/B/E/P/Y) | Assistant: No; Read aloud 2.x (C11) |
| Compare, Word Count | | Word Count N4.5 (from text layer); Compare 2.x |
| View Setting ▾ | Navigation Panels (F4), Status Bar (show/auto-hide/hide), Start Page, Full Screen (F11) | Now |

### Form, Protect, Share, Accessibility, Help

| Tab | Foxit | Atlas |
|---|---|---|
| Form | Field recognition, field authoring, templates, XFA, calculation order, reset, import/export data, JavaScript | 2.x (C20/E3) filling first; JavaScript: No |
| Protect | Redaction, Whiteout, Hidden Data, Secure Document, Fill & Sign, Sign & Certify, Time Stamp, Validate | Show security and signature *status* N5; redaction, signing 2.x (E4/E5); encryption changes: No |
| Share | Email, Evernote, OneNote, collaboration, shared/email review, Tracker | No (no cloud, no accounts) |
| Accessibility | Autotag, Full Check, Report, Setup Assistant, Reading Option (Ctrl+Shift+5), Reading Order (Ctrl+Shift+U), Alternate Text, Area order | Reading Option (reading colours, page reading settings) Now; Setup Assistant N9; Reading Order view N9; tagging/checking 2.x |
| Help | User Manual (F1), Help Center, Command Line Help, Online Tutorial, Plug-in Management, Set to Default PDF Reader, About, Deactivate | User Guide (offline) N7; Command Line Help N7; Set as default PDF reader N7; About Now; plug-ins, accounts: No |

### Navigation panels (rail)

| Foxit panel | Atlas |
|---|---|
| Bookmarks | N5 (Atlas's signature editor) |
| Pages (thumbnails) | N4.4 |
| Comments | N6 |
| Layers, Attachments, Signatures, Content, Tags, Destinations, Fields, Order, Links | Attachments and Signatures (read-only) 2.x; others 2.x candidates |
| Right panel: Search (Ctrl+Shift+F), Format | Search N4.5; Format N6 |

### Status bar

| Foxit | Atlas |
|---|---|
| First « (Ctrl+Home), Previous ‹, page box "n / total" (Ctrl+G), Next ›, Last » (End) | Now |
| Previous / next view (Alt+Left / Alt+Right) | N4.4 |
| Page layout buttons (single, continuous, facing, continuous facing) | N4.4 |
| Zoom out −, slider, zoom in +, percentage menu, Full screen ⛶ | Now |

## 4. Keyboard

Atlas adopts Foxit's default shortcuts for every command it has. Changes to Atlas's earlier map:

| Key | Before | Now (Foxit) |
|---|---|---|
| Ctrl+K | Library search | **Preferences** |
| Ctrl+F | — | Find in the current view: Library search box, or document find (N4.5) |
| Alt+Q | — | Title-bar search (N5 Command Center) |
| Ctrl+Shift+F | — | Search panel (N4.5) |
| Ctrl+Shift+A | Add library folder | Deselect all (N4.5); Add folder moves to **Ctrl+Shift+O** |
| Ctrl+Q | — | Exit |
| Ctrl+F11 | — | Collapse / pin the ribbon |
| F4 | — | Show / hide navigation panels |
| Alt+3 / Alt+6 / Alt+7 | — | Hand / Select / SnapShot (when they exist) |
| Ctrl+3 | — | Fit Visible (N4.4) |

Single-key accelerators (H hand, V select, Z zoom, …) are off by default and enabled in Preferences → General, as in Foxit (N4.5 with the tools they select).

## 5. Translation of the layout to Arabic and Urdu

Foxit has no Arabic or Urdu interface. Atlas mirrors the whole workspace in Arabic and Urdu: title bar, ribbon tab order, command order within groups, rail on the right, status bar navigation on the right, ‹ › meaning previous/next in reading direction. Ribbon labels use the translated text; buttons grow to the label (Urdu Nastaliq adds 10 px to fixed heights). Shortcuts are the same physical keys in every language.
