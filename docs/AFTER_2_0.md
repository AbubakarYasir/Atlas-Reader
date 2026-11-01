# Atlas Reader — After 2.0

These capabilities were moved out of Windows 2.0 by the **2026-11-01 focus decision** ([PLAN.md §9](../PLAN.md#9-decision-log)). Their requirements are kept so nothing is forgotten, but **none of them may start before stable `2.0.0`** unless the owner explicitly changes scope. The original, fuller specifications are preserved unchanged in [history](history/plan-v1/docs/FORMAT_EXPANSION_ROADMAP.md) ([formats](history/plan-v1/docs/FORMAT_EXPANSION_ROADMAP.md), [offline editing](history/plan-v1/docs/OFFLINE_EDITING_ROADMAP.md), [platforms](history/plan-v1/docs/PLATFORM_ROADMAP.md)).

The release order is decided at N11, based on user value. The proposed order is below.

## What 2.0 must already prepare

- Shared identity, Library, metadata, search, progress, backup and UI models stay **format-neutral** (book, not PDF), while PDF rendering, coordinates, security and writes stay in PDF adapters.
- N5 defines a versioned **location envelope** so ebook locations are never stored as screen page numbers.
- 2.0 shows honest "not supported yet" states for EPUB/comic/Kindle files, forms, XFA, attachments and signature validation.

## Track A: More reading (proposed 2.1–2.2)

| ID | Capability | Key gates |
|---|---|---|
| C17 | Side-by-side reading (same or two books), independent positions, clear active pane | Shared memory budget; keyboard pane switching; never edits the wrong document |
| C18 | Simplified text (reflow) view for qualified text PDFs, with return to the source page | Tagged multi-column, Arabic/Urdu/bidi, tables and footnotes corpus; no invented or omitted content; scans keep exact layout |
| C11 | Offline read-aloud with installed Windows voices; pause, resume, speed, start from a selection | No document transmission; honest state when no Arabic/Urdu voice is installed |

## Track B: More formats (proposed 2.2–2.4)

Every admitted format shares the Library rules: filename-first name, protected extension, the same Favorites/Recents/search/identity, distinct states for unsupported/damaged/encrypted/DRM/missing, off-thread parsing with bounded memory, and Arabic/accessibility gates. **An extension is never "supported" just because it's recognized.**

| ID | Formats | Level | Binding limits |
|---|---|---|---|
| F1 | EPUB | Reflowable, non-DRM, full reading; fixed-layout gated separately | Scripts and network disabled; layout/sanitizer chosen by ADR; durable locations after reflow |
| F2 | CBZ, CBR | Image-sequence reading, covers, spreads, RTL manga | Traversal, decompression bombs and corrupt entries tested; CBR needs a separately licensed RAR route |
| F3 | AZW3, MOBI, PRC | User-owned, DRM-free, at the fidelity proven | Content detection over extension; DRM always unsupported; established-reader comparison |
| F4 | KFX | Feasibility ADR only; support only if lawful non-DRM evidence passes | Never a DRM bypass; never advertised by extension alone |

Each format goes through: qualification (lawful fixtures, open-source candidates, licenses, threat model, capability matrix) → Library admission → reader → interoperability and preservation → owner review and `F# PASS`.

## Track C: PDF editing tools (proposed 2.4+)

All writes reuse the N5 safe-save, capability and conflict system and write to a new file by default. Each needs a qualified open-source route and an ADR before implementation. The accepted Qt PDF/qpdf split does not prove these operations.

| ID | Capability | Must never |
|---|---|---|
| C20 + E3 | AcroForm filling (text, check, radio, choice), then field authoring | Show a static form as filled; run scripts or submit; break editability |
| E1 | Page organization: reorder, insert, duplicate, delete, extract, split, merge, rotate, crop | Silently move research to the wrong page; call cropping "removal" |
| E2 | Original text/image editing; offline, sandboxed equation editor | Corrupt Arabic shaping or fonts; execute equation input |
| E4 | Secure redaction (mark → apply to a new copy) | Leave targeted content recoverable through text, objects, images or revisions |
| E5 | Visible signature marks; local-certificate cryptographic signing; offline verification with honest trust/revocation states | Leak keys; call untrusted "valid"; use the network |

## Track E: Foxit parity candidates (proposed 2.2+)

From the command-by-command map in [FOXIT_PARITY.md](FOXIT_PARITY.md). Each goes in the ribbon tab Foxit uses for it.

| ID | Capability | Foxit place | Key gates |
|---|---|---|---|
| X1 | Offline OCR for scans (English, Arabic, Urdu), searchable-text layer saved to a new copy | Convert → Recognize Text; Home → Quick Recognition | Open-source engine with Arabic/Urdu (Nastaliq) accuracy proven on a lawful corpus; never uploads; never alters the original |
| X2 | Export pages or all images to PNG/JPEG; export text to plain text/Markdown | Convert → Export All Images, To Image, To Other | Correct colour, DPI, page ranges; Arabic text order in exported text |
| X3 | Stamps and custom stamps | Comment → Stamp | Standard annotation round trip |
| X4 | Watermark, background, header & footer, Bates numbering | Organize | New file by default; preservation tests |
| X5 | Attachments, signatures, layers panels (read-only first) | Navigation rail | Never execute attachments; honest signature states |
| X6 | Rulers, line weights, compare documents, loupe/magnifier, split view | View, Home | Accessible, keyboard-operable |
| X7 | Ribbon customization (custom tabs/groups, import/export) | right-click ribbon → Customize | Settings backup includes it |

## Track D: Platforms

Order: **Android → Linux → macOS → iOS/iPadOS**, each as its own `2.x` line.

| Platform | Key adaptations |
|---|---|
| Android | Storage Access Framework/content URIs, scoped storage, lifecycle, share/open-with intents, touch/stylus density, mobile shell, background limits |
| Linux | Wayland (+ X11), portals and pickers, watcher differences, printing, distribution packaging, font/RTL consistency |
| macOS | Finder/open-with, menus, sandbox/security-scoped access, printing, Metal, trackpad, notarization |
| iOS/iPadOS | Document picker, security-scoped resources, memory pressure, sharing, Pencil, tablet shell |

A platform may use different adapters and layout, but it may never change the core rules for security, storage states, identity, conflicts or backup. If a port reveals a flawed abstraction, fix it through an ADR with cross-platform tests, not with a forked core.

## Never planned

Cloud accounts/sync, collaboration and shared/email review, AI document chat or assistant, telemetry, notifications, Office conversion, document JavaScript, measuring/accounting tools, audio notebooks, multimedia/3D, executable plugins, password cracking, restriction removal, DRM bypass. (OCR moved to Track E candidate X1 on 2026-11-01.)

## Adding an idea

Record its user value, an open specification or maintainable route, DRM/legal limits, security model, platform coverage, accessibility behavior, fixture provenance and the release that would own it. Until then it is a candidate, not a promise.
