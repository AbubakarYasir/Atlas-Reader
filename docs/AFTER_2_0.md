# Atlas Reader — After 2.0

These capabilities come after Windows 2.0: those moved out by the **2026-11-01 focus decision**, plus **every remaining Foxit PDF Editor feature** (owner direction 2026-11-01: Atlas covers all of them at some point; full list in [FOXIT_PARITY.md §3](FOXIT_PARITY.md#3-complete-foxit-feature-catalogue-and-atlas-plan)) ([PLAN.md §9](../PLAN.md#9-decision-log)). Their requirements are kept so nothing is forgotten, but **none of them may start before stable `2.0.0`** unless the owner explicitly changes scope. The original, fuller specifications are preserved unchanged in [history](history/plan-v1/docs/FORMAT_EXPANSION_ROADMAP.md) ([formats](history/plan-v1/docs/FORMAT_EXPANSION_ROADMAP.md), [offline editing](history/plan-v1/docs/OFFLINE_EDITING_ROADMAP.md), [platforms](history/plan-v1/docs/PLATFORM_ROADMAP.md)).

The release order is decided at N11, based on user value. The proposed order is below.

## What 2.0 must already prepare

- Shared identity, Library, metadata, search, progress, backup and UI models stay **format-neutral** (book, not PDF), while PDF rendering, coordinates, security and writes stay in PDF adapters.
- N5 defines a versioned **location envelope** so ebook locations are never stored as screen page numbers.
- 2.0 shows honest "not supported yet" states for EPUB/comic/Kindle files, forms, XFA, attachments and signature validation.

## How the tracks run

Each track is a release line (`2.x`) with its own stages, Automated QA, owner review and `T# PASS`, run in the same way as N4–N11. Every write reuses the N5 safe-save, capability and conflict system and writes to a **new file by default**. Each needs a qualified open-source route and an ADR before implementation; the accepted Qt PDF/qpdf split does not prove these operations. Every command lands in the ribbon tab, group and position Foxit uses for it ([FOXIT_PARITY.md](FOXIT_PARITY.md)), in English, Arabic and Urdu.

| Track | Theme | Foxit places | Proposed order |
|---|---|---|---|
| A | Reading Plus | View, Home | 1 |
| B | More formats | — | 2 |
| C | Pages, Create and Convert | Organize, Convert, File → Create/Export | 3 |
| D | Edit content and rich annotations | Edit, Comment (advanced), Measure | 4 |
| E | Forms | Form | 5 |
| F | Protect and Sign | Protect | 6 |
| G | OCR, Accessibility authoring and Standards | Convert → Recognize, Accessibility, Preflight, Optimizer | 3 (OCR early for Arabic/Urdu scans) |
| H | Automate, batch and customize | File → Action Wizard, Index, Portfolio, Binder; Customize; add-ins | 7 |
| I | Review and Share | Share, comment status, Tracker | 8 |
| J | Connected features (opt-in) | Cloud places, ECM, e-signature services, AI/Translate assistants, updates | 9 |
| K | Platforms | — | in parallel after 2.0 |

## Track A: Reading Plus

| ID | Capability | Key gates |
|---|---|---|
| C17 | Side-by-side reading and **tab groups** (new horizontal/vertical group, move tabs, split view vertical/horizontal/spreadsheet), independent positions | Shared memory budget; keyboard pane switching; never edits the wrong document |
| C18 | Reflow view and **Text Viewer** (font, colours, find) for qualified text PDFs, with return to the source page | Tagged multi-column, Arabic/Urdu/bidi, tables and footnotes corpus; no invented or omitted content |
| C11 | Offline **read aloud** with installed Windows voices (current page, from page, pause, stop, speed) | No document transmission; honest state when no Arabic/Urdu voice is installed |
| A1 | **Rulers, guides, line weights, layout grid** (units: points, picas, mm, cm, inches) | Accessible, keyboard-operable |
| A2 | **Loupe and magnifier** tools | Smooth at 60 fps on reference hardware |
| A3 | **Compare documents** (text and visual differences, results view, next/previous change Ctrl+. / Ctrl+,) | Arabic/Urdu text comparison; large-document memory |
| A4 | **Page transitions** in full screen; **multiple windows**; **dock/float panels** | Reduced-motion respected |

## Track B: More formats

Every admitted format shares the Library rules: filename-first name, protected extension, the same Favorites/Recents/search/identity, distinct states for unsupported/damaged/encrypted/DRM/missing, off-thread parsing with bounded memory, and Arabic/accessibility gates. **An extension is never "supported" just because it's recognized.**

| ID | Formats | Level | Binding limits |
|---|---|---|---|
| F1 | EPUB | Reflowable, non-DRM, full reading; fixed-layout gated separately | Scripts and network disabled; layout/sanitizer chosen by ADR; durable locations after reflow |
| F2 | CBZ, CBR | Image-sequence reading, covers, spreads, RTL manga | Traversal, decompression bombs and corrupt entries tested; CBR needs a separately licensed RAR route |
| F3 | AZW3, MOBI, PRC | User-owned, DRM-free, at the fidelity proven | Content detection over extension; DRM always unsupported; established-reader comparison |
| F4 | KFX | Feasibility ADR only; support only if lawful non-DRM evidence passes | Never a DRM bypass; never advertised by extension alone |

Each format goes through: qualification (lawful fixtures, open-source candidates, licenses, threat model, capability matrix) → Library admission → reader → interoperability and preservation → owner review and `F# PASS`.

## Track C: Pages, Create and Convert

| ID | Capability | Must never |
|---|---|---|
| E1 | **Organize:** insert (file, folder, open files, clipboard, blank, scanner), delete, extract, reverse, rearrange, move, duplicate, replace, split (document/pages), swap, interleave, rotate, crop, resize, flatten | Silently move research to the wrong page; call cropping "removal" |
| C1 | **Page furniture:** watermark, background, header & footer, Bates numbering, format page numbers (page labels), page thumbnails embed/remove | Damage existing content; lose page labels |
| C2 | **Create:** from files (images, text), clipboard, blank, template, combine files (with bookmarks per file), saved web page (offline) | Fetch remote content without consent |
| X2 | **Export:** all images, pages to PNG/JPEG/TIFF, text/Markdown/RTF; then Office (Word, Excel, PowerPoint) and HTML through an open-source converter (ADR) | Reorder Arabic/Urdu text; claim fidelity without a corpus |

## Track D: Edit content and rich annotations

| ID | Capability | Must never |
|---|---|---|
| E2 | **Edit text and objects** (text, image, shape, shading), link & join text, add text/images/shapes, reflow editing, spell check, search & replace (Ctrl+T); offline, sandboxed equation editor | Corrupt Arabic shaping or fonts; execute equation input |
| D1 | **Image editing tools:** select, magic wand, lasso, polygonal lasso, filters (dodge, greyen, sharpen, soften, binarize, convex/concave) | Change the original without safe save |
| D2 | **Links and structure:** create links, detect web links, cross references, articles and article boxes, named destinations | Create links that launch programs |
| D3 | **Advanced comments:** callout, polygon, polyline, cloud, file attachment annotation, image annotation, stamps (custom, dynamic, sequential, favourites), symbols/numbers/letters, tick marks, comment summary (PDF Summary) | Break interoperability with other readers |
| D4 | **Measure** (distance, perimeter, area, calibration) and **accounting calculator** | Wrong units on scaled drawings |
| D5 | **Attachments, layers, 3D and multimedia panels** (view, save copy; layers show/hide, overrides, merge, flatten; U3D/PRC viewer; audio/video playback) | Execute attachments; play media without consent; use the network |

## Track E: Forms

| ID | Capability | Must never |
|---|---|---|
| C20 | **Fill** AcroForms (text, check, radio, choice, date, barcode display), reset, import/export data (FDF, XFDF, XML, CSV; combine forms to a sheet) | Show a static form as filled; submit without asking |
| E3 | **Author** fields (button, check, radio, combo, list, text, signature, image, date, barcode), properties, tab order, duplicate, multiple copies, align/distribute, field recognition, page templates, calculation order, tooltips; static XFA editing | Break editability in other readers |
| E6 | **Document JavaScript**: console and editor; runs only in a sandbox, off by default, per-document trust | Access files, network or launch programs |

## Track F: Protect and Sign

| ID | Capability | Must never |
|---|---|---|
| E4 | **Redaction** (text, area, page, search & redact, pattern-based smart redact), whiteout, mosaic → apply to a new copy | Leave targeted content recoverable |
| F1 | **Hidden data** removal and **sanitize document** | Remove content the user did not choose |
| F2 | **Secure document:** password and certificate encryption, permissions, remove security (with the owner password only) | Crack, bypass or guess passwords |
| E5 | **Fill & Sign** (typed, drawn, image signatures, initials, marks); **Sign & Certify** with digital IDs; timestamps; **validate** signatures offline with honest trust/revocation states; certificate import/export; Trust Manager | Leak keys; call untrusted "valid"; contact servers without consent |

## Track G: OCR, Accessibility authoring and Standards

| ID | Capability | Key gates |
|---|---|---|
| X1 | **Offline OCR** (Recognize Text, Quick Recognition, Suspect Results, scanner input with presets) for English, Arabic and **Urdu Nastaliq** | Open-source engine; accuracy proven on a lawful corpus; never uploads; new copy by default |
| G1 | **Accessibility authoring:** autotag document and form fields, full check, accessibility report (PDF/UA), reading order and area order editing, alternate text, tags/content/order panels | Never claim compliance without the checker passing |
| G2 | **Standards and size:** PDF/A conversion and validation, preflight profiles and reports, PDF Optimizer (reduce size, advanced, optimize scanned) | Silent quality loss; claim conformance without validation |

## Track H: Automate, batch and customize

| ID | Capability | Key gates |
|---|---|---|
| H1 | **Action Wizard** (create, manage, run sequences of commands on files and folders) and batch operations (batch print, convert multiple files, batch OCR) | Every batch step previewable and reversible; new files by default |
| H2 | **Customize ribbon and shortcuts:** custom tabs and groups, rename, reorder, import/export, reset; keyboard customization | Settings backup includes it |
| H3 | **Portfolio and Binder** (create, add files/folders, cover sheet, extract), **embedded indexes** and save search results to PDF/CSV | Open embedded files only as copies |
| H4 | **Windows integration:** Explorer thumbnails and preview handler, right-click convert/combine, Office and browser add-ins | Installer remains removable cleanly |
| H5 | **Plug-ins** with a signed, sandboxed model (ADR) | Run unsigned code; read files outside a grant |

## Track I: Review and Share

| ID | Capability | Key gates |
|---|---|---|
| I1 | **E-mail** current/all tabs and comments (FDF) through the local mail app | Never sends silently |
| I2 | **Shared review** through a shared folder (network or synced drive) and **e-mail review**: invite, join, publish/merge comments, deadlines, Tracker | No Atlas server; conflict-safe merges |
| I3 | **Comment workflow:** status (accepted, rejected, cancelled, completed, deferred, future), checkmarks, labels, author names, sort and filter | Interoperable with other readers' review states |

## Track J: Connected features (opt-in)

Atlas stays **offline by default**. Each connected feature is off until the user turns it on, says exactly what leaves the computer, works without an Atlas account, and can be removed without losing local research.

| ID | Capability | Key gates |
|---|---|---|
| J1 | **Cloud places** in File → Open/Save: OneDrive, Google Drive, Dropbox, Box, Egnyte, ShareFile (local sync folders already work in 2.0) | Official APIs; tokens in Windows Credential Manager; offline copies stay usable |
| J2 | **Document management systems:** SharePoint, iManage, NetDocuments, Documentum, OpenText, Alfresco, Worldox (check in/out, properties) | Enterprise ADR per connector |
| J3 | **E-signature services** (DocuSign and similar) and Microsoft Information Protection / RMS / AIP labels, security watermarks, batch encrypt | No credential storage outside the OS vault |
| J4 | **Assistants:** summarize, ask, translate — **on-device models first**; a cloud model only with explicit per-document consent | Never uploads by default; Arabic/Urdu quality gate |
| J5 | **Updates** check, online help and tutorials, notifications | Opt-in; signed updates |

## Track K: Platforms

Order: **Android → Linux → macOS → iOS/iPadOS**, each as its own `2.x` line.

| Platform | Key adaptations |
|---|---|
| Android | Storage Access Framework/content URIs, scoped storage, lifecycle, share/open-with intents, touch/stylus density, mobile shell, background limits |
| Linux | Wayland (+ X11), portals and pickers, watcher differences, printing, distribution packaging, font/RTL consistency |
| macOS | Finder/open-with, menus, sandbox/security-scoped access, printing, Metal, trackpad, notarization |
| iOS/iPadOS | Document picker, security-scoped resources, memory pressure, sharing, Pencil, tablet shell |

A platform may use different adapters and layout, but it may never change the core rules for security, storage states, identity, conflicts or backup. If a port reveals a flawed abstraction, fix it through an ADR with cross-platform tests, not with a forked core.

## Never

Things that break the law, a document's protection or the user's trust are never built: password cracking, removing restrictions without the owner password, DRM bypass, hidden telemetry or uploads without consent, executable plug-ins without signing and a sandbox. Everything else Foxit offers is planned above.

## Adding an idea

Record its user value, an open specification or maintainable route, DRM/legal limits, security model, platform coverage, accessibility behavior, fixture provenance and the release that would own it. Until then it is a candidate, not a promise.
