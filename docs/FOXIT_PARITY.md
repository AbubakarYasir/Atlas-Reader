# Atlas Reader — Foxit Workspace Parity

<!-- Living reference. Decision: ADR-0007. -->

**Owner decision (2026-11-01):** Atlas copies the *layout, measurements, command placement, interaction model and keyboard shortcuts* of Foxit PDF Editor (Windows, ribbon mode, version installed on the owner's PC) as closely as possible, so the product feels like a mature desktop PDF tool instead of a generic "AI-looking" app. **Every Foxit feature will be covered**: what a reader and research tool needs before 2.0, the rest in the after-2.0 tracks (§3) ([ADR-0007](decisions/ADR-0007-foxit-workspace-parity.md)).

**What is copied:** structure, sizes, spacing, order of tabs, groups and commands, labels (translated), menus, panels, status bar, File page, Preferences window, skins model, Touch mode, shortcuts and single-key accelerators.

**What is not copied:** Foxit's icon artwork, logos, skin images, colours used as brand (purple), text of their manual, or any file from `C:\Program Files\Foxit Software`. Those are Foxit's copyrighted assets and the licence forbids extracting them. Atlas uses Fluent UI System Icons (MIT, thin-line, same 16/20/24 px grid), its own colours and its own name.

**Sources:** the owner's Foxit UI reference package (`foxit-ui-ux-reference-2026.1.1.36485`: 26 runtime screenshots, light/dark skin colour definitions, 114 English menus, 159 core dialog titles, 107 modules and 62 plug-ins; used for measurements, colour values and the feature inventory only); 14 owner screenshots of Foxit PDF Editor at 100% scale (1919 × 1044 window), measured with pixel analysis on 2026-11-01; Foxit PDF Editor User Manual (workspace, skins, Touch mode, navigation pane, status bar, keyboard shortcuts appendix). Measurements are in device-independent pixels at 100%.

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

Neutral colours measured (Atlas uses these values for its neutral roles, [DESIGN.md §4.1](DESIGN.md); the accent stays Atlas's):

| Role | Foxit light | Atlas light (dark) |
|---|---|---|
| Ribbon surface | `#FBFBFD` | `#fbfbfd` (`#333333`) |
| Rails, File page list | `#F5F6FA` | `#f5f6fa` (`#424242`) |
| Gaps, inactive tabs | `#F0F1F5` | `#f0f1f5` (`#242424` / `#424242`) |
| Lines | `#C7CCD9` | `#c7ccd9` (`#131313`) |
| Hover / pressed / separators | `#E6E9F0` | `#e6e9f0` (`#424242`) |
| Title bar (Colourful skin) | brand purple | Atlas green `#28633A` or the chosen theme colour |

## 2. Skins

Foxit: **File → Skins**, mode Light / Dark / Use system setting; in Light mode a *Colourful* option tints the title bar with a theme colour. Atlas copies this model:

- **Mode:** Light, Dark, System.
- **Light theme:** *Classic* (neutral title bar) or *Colourful* (title bar in the theme colour). Theme colours come from the **Atlas logo** (§2.1): Atlas Blue (default), Midnight, Sky, Teal, Plum, Graphite. Never Foxit purple.
- **Touch mode** button on the Quick Access Toolbar: larger targets in ribbon and toolbar (+8 px).
- Reading colours (Paper, Sepia, Night, High contrast) stay in **Accessibility → Reading Option** and View, like Foxit's "Replace document colours".

### 2.1 Atlas colours from the logo

Atlas's own colour is built from its logo (`assets/branding/atlas-reader-icon.png`): deep navy globe, royal-blue pages, cyan bookmark and compass, ice-white paper. Foxit's neutral greys stay; Foxit's purple is never used.

| Role | Light | Dark | From the logo | Contrast |
|---|---|---|---|---|
| `accent` (selected ribbon tab, primary buttons, focus, links) | `#0b4fae` Atlas Blue | `#6cc4ff` Sky | Royal-blue pages / cyan star | 7.7:1 on white · 6.6:1 on `#333333` |
| `accentText` (text on accent) | `#ffffff` | `#131313` | | 7.7:1 · 9.7:1 |
| `accentSoft` (selected rows, info banners) | `#e3f1fb` | `#123a66` | Ice paper / navy | accent text ≥ 6.3:1 |
| `brandDeep` (title bar in Colourful skin) | `#0b3d91` | — | Navy globe | white 10:1 |
| `highlight` (decorative only: bookmark marks, progress) | `#00a0f8` | `#00a0f8` | Cyan bookmark | not for text |

Colourful title-bar choices: **Atlas Blue** `#0b3d91` (default), **Midnight** `#0a2f6e`, **Sky** `#0b4fae`, **Teal** `#155e75`, **Plum** `#6b3a6b`, **Graphite** `#3c4148`; all keep white text ≥ 7:1.

## 3. Complete Foxit feature catalogue and Atlas plan

**Owner direction (2026-11-01):** Atlas will cover **every feature** of Foxit PDF Editor 2026.1 at some point. This section lists all of them, from the owner's reference package (11 ribbon tabs, the classic menu bar, 114 menus, 159 core dialogs, 107 modules and 62 plug-ins) plus the manual, and gives each one a home:

- **Done** — shipped in N4.1–N4.3.
- **N4.4 … N10** — a stage before Windows 2.0 ([CHECKPOINTS.md](../CHECKPOINTS.md)).
- **Track A–K** — after 2.0, in the tracks of [AFTER_2_0.md](AFTER_2_0.md). Track order is decided at N11 from user value.

A command appears in Atlas only when it works; until then its group is absent, never greyed out as a promise. Every feature keeps Atlas's rules: offline by default, nothing changes a PDF without the safe-save path, protected files are never bypassed, all three languages, keyboard and Narrator.

### 3.1 Window, title bar, tabs, panels, status bar

| Foxit feature | Where in Foxit | Atlas |
|---|---|---|
| Custom title bar, file name, window buttons | Title bar | Done (N4.3) |
| Quick Access Toolbar: Open, Save, Print, Create, Undo, Redo, Hand/Select, Mouse/Touch | Title bar | Done: Open, Mouse/Touch · N5: Save, Undo, Redo · N7: Print · Hand/Select on the Home tab Done (N4.5) |
| Customize Quick Access Toolbar (add, remove, order, above/below ribbon) | ▾ on QAT, right-click ribbon | N7 |
| Customize ribbon (new tabs/groups, rename, reorder, import/export, reset) and keyboard shortcuts | Right-click ribbon → Customize | Track H |
| Title-bar search (commands, text, help; Alt+Q), collapsed search option | Title bar | N5 (Command Center) |
| Notifications bell | Title bar | Track J (local notices only before that) |
| Ribbon collapse / pin (Ctrl+F11), double-click a tab | Ribbon | Done (N4.3) |
| Classic menu bar (File, Edit, View, Tools, Help) as an option | View → Menu Bar | N7 (keyboard Alt menus) |
| Document tabs: Ctrl+Tab, middle-click close, overflow list | Tab bar | Done (N4.3) |
| Tab context menu: Close, Close Others, Close Left, Close Right, Close All, Copy Path, Open Containing Folder, Print Current/All Tabs | Right-click tab | Done (N4.4) · Print items N7 |
| Tab groups: New Horizontal/Vertical Tab Group, move tabs between groups | Right-click tab | Track A (C17 side-by-side) |
| Start page with recent files and tool wizard | Start tab | Done as the Library tab; tool wizard Track H |
| Navigation rail and panels; F4; Reset All Panels; dock/float panels; resize | Left pane | Rail, Bookmarks and Pages panels, Reset All Panels Done (N4.4/N4.5) · float/dock Track A |
| Right panel (Search, Format) and Right Panel Switch | Right side | Search Done (N4.5) · N6 (Format) |
| Status bar: page navigation, previous/next view, page layouts, zoom slider, zoom menu (1%–6400%), full screen; show / auto-hide / hide | Bottom | Done: navigation, zoom, full screen, show/auto-hide/hide · previous/next view, layouts, full zoom list Done (N4.4) |
| Mouse/Touch mode | QAT | Done (N4.3) |
| Skins: Light / Dark / System; Colourful theme colour | File → Skins | Done (N4.3) |
| Minimize to tray; tray menu | System tray | N7 |
| Multiple windows / single-document reading | Preferences → Documents | Track A |

### 3.2 File page and file handling

| Foxit feature | Atlas |
|---|---|
| Open → Recent Documents (pin, open a copy, copy path, remove), Computer, cloud places | Done: recent, Computer, remove · pin, copy path, open containing folder Done (N4.4) · open a copy N7 · cloud places Track J |
| Create (blank, from file, scanner, clipboard, web page, template, portfolio, binder) | Track C (portfolio/binder Track H) |
| Properties (Ctrl+D): description, security, fonts, initial view, custom, advanced, XMP | N7 (view and permitted edits) |
| Save (Ctrl+S), Save As (Ctrl+Shift+S), Revert | N5 safe save |
| Print (Ctrl+P), Print preview, Batch Print, print current/all tabs | N7 (batch print Track H) |
| Share: e-mail current/all tabs (Ctrl+Shift+L / K) | Track I |
| Export (to images, Office, HTML, text) | Track C |
| Close, Close All, Exit | Done |
| PDF Optimizer: reduce file size, advanced optimization, optimize scanned PDF, multiple files | Track G |
| Index: create, manage embedded index, search across indexed PDFs | Library search already covers this need · embedded indexes Track H |
| Action Wizard: create, manage, run actions (batch steps) | Track H |
| Preferences (Ctrl+K) | Done: General, Languages, Skins, Accessibility · each stage adds its categories (below) |

### 3.3 Preferences categories (all 34)

3D · Accessibility · Action Wizard · Commenting · Create PDF · Documents · ECM Integration · Editing · File Associations · Forms · Full Screen · General · History · Identity · Index · JavaScript · Languages · Layout Grid · Measuring · Multimedia · Page Display · PDF Sign · Print · Reading · Reviewing · Search · Security · Signature · Speech · Spelling · Tablet · Text Recognition · Trust Manager · Updater.

| Atlas stage | Categories it adds |
|---|---|
| Done (N4.3) | General, Languages, Skins, Accessibility |
| N4.4 | Done: Page Display (default layout, default zoom), History (clear; pinned files kept), Documents (restore tabs, restore last view per book) · N7: Full Screen options, rendering options |
| N4.5 | Done: Search (whole words, case-sensitive, Arabic/Urdu note) · N7: Reading (reading order, caret browsing) |
| N5 | Security (protected files, local research), Identity (author name for notes and bookmarks) |
| N6 | Commenting, Tablet (pen and touch) |
| N7 | Print, File Associations, Library (Atlas), Backup (N8) |
| N9 | Accessibility completed, Speech (when read-aloud lands, Track A) |
| Tracks | Editing, Spelling, Layout Grid (D) · Forms, JavaScript (E) · PDF Sign, Signature, Trust Manager (F) · Text Recognition, Create PDF (C/G) · Action Wizard, Index (H) · Reviewing (I) · ECM Integration, Updater (J) · Measuring, 3D, Multimedia (D) |

### 3.4 Home

| Group | Foxit commands | Atlas |
|---|---|---|
| Tools | Hand (Alt+3), Select Text and Image (Alt+6), Select Annotation, SnapShot (Alt+7), Clipboard ▾ (copy, paste, paste in place), Bookmark | Hand Done · Select Text, SnapShot, Clipboard (copy, select all, deselect) Done (N4.5) · paste N6 · Select annotation N6 · Bookmark N5 |
| View | Zoom ▾ (in, out, marquee, loupe, magnifier, Zoom To Ctrl+M), Page Fit Option ▾ (Actual Size Ctrl+1, Fit Page Ctrl+0, Fit Width Ctrl+2, Fit Visible Ctrl+3), Reflow (Ctrl+4), Rotate View ▾ | Zoom, Page Fit, Rotate View Done · Fit Visible, Zoom To, marquee zoom, full zoom list Done (N4.4) · loupe, magnifier, Reflow Track A |
| Edit | Edit Text, Edit Object ▾ | Track D |
| Comment | Typewriter, Highlight | N6 |
| Pages | Rotate Pages ▾, Insert ▾ | Track C |
| Scan | From Scanner ▾, Quick Recognition | Track G |
| Sign | Fill & Sign | Track F |

### 3.5 Convert

| Group | Foxit commands | Atlas |
|---|---|---|
| Create | From Files, From Scanner, From Clipboard, From Web Page, Form, PDF Portfolio, Make Binder, Combine Files, Blank, From Template | Files, Clipboard, Combine, Blank, Template Track C · Scanner Track G · Web Page Track C (offline snapshot of a local or saved page) · Portfolio, Binder Track H |
| Export | Export All Images, To MS Office (Word, Excel, PowerPoint), To Image, To HTML, To Other (text, RTF, XPS, PDF/A…) | Track C (text, Markdown, images first; Office and HTML through an open-source converter, ADR needed) |
| Recognize | Recognize Text, Quick Recognition, Suspect Results | Track G (English, Arabic, Urdu Nastaliq) |
| Preflight | Preflight | Track G |

### 3.6 Edit

| Group | Foxit commands | Atlas |
|---|---|---|
| Content | Edit Text, Edit Object (text, image, shape, shading), Link & Join Text, Check Spelling, Search & Replace (Ctrl+T) | Track D |
| Add | Add Text, Add Images, Add Shapes, Reflow Editing, Add Article Box, Articles panel | Track D |
| Links | Web Links ▾ (create from URLs, remove all), Link, Bookmark (Ctrl+B), Auto Create Bookmarks, Cross Reference (Ctrl+Shift+C) | Following links Done (N4.5) · Bookmark N5 · Auto Create Bookmarks from outline/headings N5 · creating links, web-link detection, cross reference Track D |
| Insert | File Attachment, Image Annotation, Audio & Video, Add 3D | Attachments panel (read, save copy) Track D · image annotation Track D · audio/video and 3D playback Track D (sandboxed, off by default) |
| Image editing | Page Editor tools: magic wand, lasso, polygonal lasso, filters (dodge, greyen, sharpen, binarize, soften…), add image | Track D |

### 3.7 Organize (all page operations write to a new file by default)

Insert (file, folder, open files, scanner, clipboard, blank, web page) · Delete · Extract · Reverse · Rearrange · Move · Duplicate · Replace · Split (document, pages) · Swap · Interleaving · Rotate Pages · Crop Pages · Resize Pages · Flatten · Watermark · Background · Header & Footer · Bates Numbering · Format Page Numbers (page labels) · Page Transitions · Pages panel context menu (enlarge/reduce thumbnails, embed/remove thumbnails, properties).

| Atlas |
|---|
| Reading page labels and Pages panel Done (N4.4) · thumbnail size N7 · every other Organize command Track C (Page Transitions Track A for full-screen presentations) |

### 3.8 Comment

| Group | Foxit commands | Atlas |
|---|---|---|
| Text markup | Highlight (Shift+H), Underline (Ctrl+U), Squiggly (Shift+U), Strikeout (Shift+S), Replace Text (Ctrl+Shift+R), Insert Text (Shift+I) | Highlight, underline, strikeout N6 · squiggly, replace/insert text N6 (display) and Track D (authoring) |
| Pin | Note (Ctrl+Shift+N), File | Note N6 · File Track D |
| Typewriter | Typewriter (Shift+T), Textbox (Shift+E), Callout (Shift+C) | Typewriter, Textbox N6 · Callout Track D |
| Drawing | Line, Arrow, Rectangle, Oval, Polygon, Polyline, Cloud, Pencil (Shift+P), Eraser | Line, arrow, rectangle, oval, pencil, eraser N6 · polygon, polyline, cloud Track D |
| Area | Area Highlight, Search & Highlight | Area highlight N6 · Search & Highlight N6 |
| Measure | Distance, Perimeter, Area, calibration; Accounting Calculator | Track D |
| Stamp | Stamps, Custom Stamp, Dynamic Stamp, Sequential Stamp, Manage/Favourite Stamps | Track D |
| Symbols | Symbols, Numbers, Letters, TickMark (previous/next) | Track D |
| Manage | Summarize Comments (PDF Summary), Import, Export (Ctrl+L, FDF/XFDF/CSV), Export Highlighted Text (Ctrl+I), FDF via Email, Manage Comments: show/hide all (Ctrl+8 / Ctrl+Shift+8), open/close all pop-ups, Keep Tool Selected | Export (JSON/Markdown + FDF/XFDF), show/hide, pop-ups, Keep Tool Selected, Export Highlighted Text N6 · Summarize Track D · FDF via e-mail Track I |
| Comment context menu | Set Status (None, Accepted, Rejected, Cancelled, Completed, Deferred, Future), checkmark, labels, reply, group/ungroup, properties, set as default | Reply, properties, set as default N6 · status, checkmark, labels Track I |
| Comments panel | Sort by page, type, author, date, status, checkmark, colour; filter; search | N6 |

### 3.9 View

| Group | Foxit commands | Atlas |
|---|---|---|
| Modes | Read Mode (Ctrl+H), Full Screen (F11), Reflow, Reverse View, Night Mode, Visual Mode ▾, Text Viewer (Ctrl+6, colours, font, find) | Read Mode, Full Screen, Visual Mode (reading colours incl. Night) Done · Reverse View Done (N4.4) · Reflow, Text Viewer Track A |
| Rotate | Rotate View ▾ | Done |
| Ruler | Rulers (units: points, picas, mm, cm, inches), Guides, Line Weights (Ctrl+5), Layout Grid | Track A |
| Page display | Single Page, Continuous, Facing, Continuous Facing, Show Cover Page, Split (vertical, horizontal, spreadsheet) | Layouts Done (N4.4) · Split Track A |
| AutoScroll | AutoScroll (Ctrl+Shift+H), speed, Esc stops | Done (N4.4) |
| Page Transitions | Full-screen transitions | Track A |
| Assistant | AI Assistant, Translate Assistant | Track J (local model first, opt-in) |
| Read | Read aloud (current page, from page, pause, stop; Ctrl+Shift+V/B/P/E/Y) | Track A (C11) |
| Compare | Compare documents (text and visual), results tab | Track A |
| Word Count | Word Count | Done (N4.5) |
| View Setting | Navigation Panels, Status Bar, Start Page, Full Screen | Done (Status Bar, Full Screen, Library) · Navigation Panels Done (N4.4) |

### 3.10 Form

| Foxit commands | Atlas |
|---|---|
| Fill fields, reset form (Reset a Form), import/export form data (FDF, XFDF, XML, CSV, combine forms to a sheet), submit (honest offline state) | Track E |
| Run Form Field Recognition, Designer Assistant | Track E |
| Field tools: push button, check box, radio button, combo box, list box, text field, signature field, image field, date field, barcode field; properties, tab order, duplicate, multiple copies, align/distribute/size, place on multiple pages | Track E |
| Page Templates, Edit Static XFA Form, Calculation Order, Add Tooltip | Track E |
| JavaScript (document JavaScript, console, editor) | Track E, sandboxed and off by default; never runs in 2.0 |

### 3.11 Protect

| Foxit commands | Atlas |
|---|---|
| Show security, permissions, signature status of a file | N5 |
| Mark for Redaction (text, area, page, search & redact, smart redact patterns), Apply, Whiteout, Mosaic | Track F (to a new copy, verified unrecoverable) |
| Hidden Data: remove hidden information, sanitize document | Track F |
| Secure Document: password protection, certificate protection, permissions, remove security (with the password only), security policies | Track F (never bypasses; removing security needs the owner password) |
| Fill & Sign (typed, drawn, image signature; initials; check marks) | Track F |
| Sign & Certify (digital IDs, certify, timestamp, validate, export certificates, trusted certificates, Trust Manager) | Track F (offline validation with honest trust states) |
| Microsoft Information Protection / RMS / AIP labels, Batch Encrypt, Security Watermark | Track J (enterprise, opt-in) |

### 3.12 Share

| Foxit commands | Atlas |
|---|---|
| E-mail current tab / all tabs through the local mail app | Track I |
| Send for Shared Review (shared network folder or service), Send for Email Review, join, merge comments, Tracker (sent/joined reviews, deadlines, reviewers) | Track I (shared folder and e-mail first; no Atlas server) |
| Evernote, OneNote, cloud collaboration, PDF Collaboration links | Track J (opt-in connectors) |

### 3.13 Accessibility

| Foxit commands | Atlas |
|---|---|
| Reading Option (Ctrl+Shift+5): replace document colours, reading order, page vs document reading | Done: reading colours · N9: reading order and page/document options |
| Setup Assistant | N9 (first-run accessibility settings) |
| Reading Order (Ctrl+Shift+U), Set Alternate Text, Set Area Reading Order, Change/Apply Area Order | View of reading order N9 · editing Track G |
| Autotag Document, Autotag Form Field, Full Check, Accessibility Report, PDF/UA | Track G |
| Untagged document read with assistive technology prompt | Page text exposed to screen readers Done (N4.5, C04) · untagged-document prompt N10 |

### 3.14 Help

| Foxit commands | Atlas |
|---|---|
| About | Done |
| User Manual (F1, offline), Command Line Help, Set to Default PDF Reader, Report Error (local diagnostic bundle) | N7 |
| Help Center, Online Tutorial, Foxit homepage, Registration, Buy, Deactivate, licence manager | Not applicable (Atlas is free and has no licence server); online help links Track J |
| Plug-in Management | Track H (signed, sandboxed plug-in model; ADR needed) |
| Check for updates (Updater) | Track J (opt-in) |

### 3.15 Navigation panels (all)

| Panel | Atlas |
|---|---|
| Pages (thumbnails, page operations menu) | Done (view, N4.4) · Track C (operations) |
| Bookmarks (Atlas's signature editor) | N5 |
| Outline / document bookmarks (read) | Done (N4.5) |
| Comments | N6 |
| Search (right panel; whole words, case, include bookmarks/comments, highlight all, save results to PDF/CSV) | Done: whole words, case, highlight all (always on), results with context (N4.5) · include bookmarks/comments N6 · save results Track H |
| Attachments | Track D (read and save copy first) |
| Layers (show/hide, list for all/visible pages, reset, print/export overrides, import, merge, flatten) | Track D (view first) |
| Signatures | N5 (status) · Track F (validation) |
| Destinations (named positions; new, sort by name/page) | Following named destinations Done (N4.5) · Destinations panel N7 · Track D (create) |
| Content, Tags, Order, Fields, Links, Articles, Model Tree (3D) | Track G (Tags, Order, Content) · Track E (Fields) · Track D (Links, Articles, 3D) |

### 3.16 Context menus

| Foxit menu | Atlas |
|---|---|
| Page background (paste, add bookmark, read mode, zoom, fit, rotate, reverse view, print, rulers, navigation panels, look up, preferences) | Done for current features (N4.4/N4.5); more items appear as their features land |
| Document tab (close, others, left, right, copy path, open folder, print, tab groups) | Done (N4.4) · print N7 · tab groups Track A |
| Text selection (copy, highlight, underline, strikeout, add bookmark, search) | copy and search Done (N4.5) · bookmark N5 · markup N6 |
| Link (open in browser after confirmation, copy link location; open in tab/new document for PDFs) | Done (N4.5) · open PDF links in a tab N7 |
| Image (copy, save image as, bookmark, extract text, edit image) | N7 (copy, save as; SnapShot copies any area now) · Track D/G |
| Recent file (open, open a copy, pin, copy path, remove) | Done (N4.4) · open a copy N7 |
| Zoom list (1%–6400%, fit options, Zoom To) | Done (N4.4) |
| Ribbon (customize QAT, customize ribbon, minimize the ribbon) | N7 / Track H |
| Navigation pane (show panels, reset all panels) | Done (N4.4/N4.5) |

### 3.17 Modules with no single ribbon home

| Module | Atlas |
|---|---|
| Cloud drives (OneDrive, Google Drive, Dropbox, Box, Egnyte, ShareFile, Foxit Drive) and ECM/DMS (SharePoint, iManage, NetDocuments, Documentum, OpenText, Alfresco, Worldox) | Track J: opt-in connectors; local sync folders already work as normal library folders in 2.0 |
| E-signature services (DocuSign, Foxit eSign) | Track J |
| Accounts, subscription, licence manager, crash upload, analytics | Not applicable: Atlas has no accounts or licences; crash reports stay a local, user-previewed bundle (N7) |
| Office add-ins (Word, Excel, PowerPoint, Outlook, Visio, Project), browser add-ins, shell extensions (preview, thumbnails, right-click convert) | Track H (Explorer thumbnails/preview first) |
| Speech (read aloud) | Track A |
| 3D (U3D/PRC) viewer | Track D |
| Portfolio, Binder | Track H |
| Calculator | Track D (with Measure) |
| Translate Assistant, AI Assistant, IDoxAI | Track J (on-device first, opt-in, never uploads without explicit consent) |
| PDF/A, preflight, optimizer | Track G |

### 3.18 Dialog reference

The 159 core Foxit dialogs (Go to Page, Zoom To, Document Information, Batch Print, Create Link, Custom/Dynamic/Sequential Stamp, Calibration, Word Count, Text Viewer Settings, Security Settings, Password, Permission Settings, Optimization Options, Scan presets, JavaScript Console, Preflight, …) are the layout reference for Atlas's equivalent dialogs when their feature lands: same title, field order and OK/Cancel placement, Atlas styling, three languages. The owner's package keeps their reconstructed layouts for side-by-side review.

## 4. Keyboard

Atlas adopts Foxit's default shortcuts for every command it has. Changes to Atlas's earlier map:

| Key | Before | Now (Foxit) |
|---|---|---|
| Ctrl+K | Library search | **Preferences** |
| Ctrl+F | — | Find in the current view: Library search box, or document Find bar (done) |
| Alt+Q | — | Title-bar search (N5 Command Center) |
| Ctrl+Shift+F | — | Search panel (done) |
| Ctrl+Shift+A | Add library folder | Deselect all in the reader (done); Add folder moves to **Ctrl+Shift+O** |
| Ctrl+Q | — | Exit |
| Ctrl+F11 | — | Collapse / pin the ribbon |
| F4 | — | Show / hide navigation panels |
| Alt+3 / Alt+6 / Alt+7 | — | Hand / Select Text / SnapShot (done) |
| Ctrl+3 | — | Fit Visible (done) |
| Ctrl+M | — | Zoom To (done) |
| F3 / Shift+F3 | — | Next / previous result (done) |
| Ctrl+C / Ctrl+A | — | Copy selection / select all on the page (done) |
| Ctrl+Shift+H | — | AutoScroll; Up/Down change speed, Esc or a click stops (done) |
| Alt+Left / Alt+Right | — | Previous / Next View (done) |

Single-key accelerators (H hand, V select, Z zoom, …) are off by default and enabled in Preferences → General, as in Foxit (N7).

## 5. Translation of the layout to Arabic and Urdu

Foxit has no Arabic or Urdu interface. Atlas mirrors the whole workspace in Arabic and Urdu: title bar, ribbon tab order, command order within groups, rail on the right, status bar navigation on the right, ‹ › meaning previous/next in reading direction. Ribbon labels use the translated text; buttons grow to the label (Urdu Nastaliq adds 10 px to fixed heights). Shortcuts are the same physical keys in every language.
