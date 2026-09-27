# Atlas Reader — Book-format expansion roadmap

<!-- atlas-status: N3|ready-for-owner-test -->

## Purpose

Atlas 2.0 remains PDF-first in implementation order, but it is now a multi-format release. This roadmap prevents the added formats from becoming an improvised collection of partial readers. Every admitted format must keep the same clear Library behavior while receiving format-specific reading, security, performance, accessibility and preservation tests.

This document plans support; it does not claim that the current N3 build opens anything except PDF.

## Shared behavior for every admitted format

- The current filename stem is the large, dependable Library name.
- The extension is a separate protected type and is preserved during a normal rename.
- User-set or embedded title and author remain editable secondary metadata.
- All books use the same Favorites, Recents, folders, search, availability and identity rules.
- The Library never says a file is readable merely because its extension is recognized.
- Unsupported, damaged, encrypted, DRM-protected and missing files receive distinct honest states.
- Parsing, cover generation and indexing stay off the interface thread and obey bounded memory/time limits.
- Arabic, Urdu, mixed-direction text, keyboard access, screen-reader names, text scaling and high contrast are acceptance gates.

## Planned admission order

| Track | Formats | Intended level | Why this order | Binding limitation |
|---|---|---|---|---|
| **F1 — EPUB** | `.epub` | Full reading for non-DRM reflowable EPUB; fixed-layout EPUB only after separate proof | Open standard and the strongest complement to fixed-layout PDF | No DRM bypass; scripting/network content disabled by default; rendering and sanitization engine must be selected by ADR |
| **F2 — Comics** | `.cbz`, `.cbr` | Image-sequence reading, covers and navigation | Clear local/offline model and strong reuse of the virtualized reader | Archive traversal, decompression bombs, corrupt entries and filename ordering must pass hostile-input tests; CBR needs a separately licensed/qualified RAR route |
| **F3 — Legacy Kindle** | `.azw3`, `.mobi`, `.prc` | Reading for user-owned, DRM-free AZW3, MOBI and PRC files when the parser can prove compatibility | Preserves legitimate user-owned older libraries | DRM-protected books remain unsupported; misleading file extensions do not override content detection; typography and metadata fidelity require established-reader comparisons |
| **F4 — KFX decision** | `.kfx` | Mandatory pre-2.0 feasibility/ADR decision; reading is admitted only if the evidence passes | KFX is proprietary, versioned and commonly tied to Amazon delivery/DRM | No DRM bypass and no false promise: a lawful non-DRM corpus, maintainable parser route, license review and owner approval are required before implementation |

## Required checkpoint sequence

This scope adds three implementation stages and a KFX decision; no release date is promised. If F4 admits KFX, implementation and an additional owner test must pass before N8; feasibility alone never means support. Missing required capabilities block the stage unless the owner explicitly changes scope.

F1/F3 include local bookmarks/notes with durable locations, text search, progress and metadata overrides. Ebook source editing and portable annotation writing require separate qualification. F2 includes page-based local bookmarks/notes and progress; no text-search/read-aloud claim is made for image-only content. N8 mixed-format backups, N9 document/UI accessibility and N10 resource/security tests cover all admitted types.

F1–F3 run after N7 and before N8, so migration, final Arabic/accessibility qualification and RC testing cover every admitted format. F4 is a mandatory decision gate in the same window; it may admit lawful non-DRM KFX or record it as unsupported with exact evidence. It may never authorize DRM bypass.

While N4–N7 are built, their shared book models must preserve the boundaries needed by F1–F3 and N8–N11. This means using generic document identity, filename/type, metadata, search, progress, backup and UI contracts where the behavior is genuinely shared. It does **not** mean weakening or generalizing PDF rendering, coordinates, security or safe-write rules: those remain strong PDF-specific adapters until another format earns its own adapter.

1. **Qualification:** representative lawful fixtures, parser/renderer candidates, license/notices, threat model, metadata/cover/text/navigation capability matrix, and repeatable performance evidence.
2. **Library admission:** content-aware detection, explicit availability/error states, metadata extraction, protected extension behavior, identity/move/rename safety and bounded cover generation.
3. **Reader delivery:** navigation, progress, search where meaningful, typography/layout, themes where safe, RTL/bidi, accessibility, keyboard/touch and large-book virtualization.
4. **Interoperability and preservation:** compare against established readers, reopen after app restart/upgrade, confirm source bytes remain unchanged unless an explicit safe write feature exists, and verify export/backup behavior.
5. **Owner gate:** distribute one exact package with checksums and a simple format-specific test sheet. The track remains Ready for owner test until the owner records `F1 PASS`, `F2 PASS` or `F3 PASS`; the KFX decision requires explicit `F4 PASS` whether its evidence admits or rejects implementation.

## Open-source dependency rule

- Prefer open specifications and actively maintained open-source parsers/renderers with reproducible builds.
- Record exact source, version/commit, license, notices, security history, update path and rollback before adoption.
- Keep each format implementation behind Atlas-owned adapters so one dependency cannot own the Library or research data model.
- Do not add cloud/account requirements to core local reading.
- A proprietary SDK is not the default answer. Any exception requires an ADR proving why no credible open route exists, what lock-in it creates, and how Atlas can remove it.
- Copyleft obligations, archive-codec licensing and bundled-font rights are release blockers, not paperwork for later.

## Minimum QA corpus per format

- ordinary small and large books;
- Arabic, Urdu, English and mixed-direction metadata/content where the format permits it;
- long Unicode filenames and duplicate visible titles;
- missing, truncated and structurally malformed files;
- encrypted/DRM-marked samples that must fail honestly without bypass attempts;
- hostile archives/resources, oversized entries, deep nesting and decompression limits where applicable;
- images, tables, links, outlines/TOC, footnotes and fonts representative of the format;
- slow/removable storage, restart, move/rename, copy and replacement scenarios;
- narrow/wide layouts, scaling, keyboard-only use, screen reader and high contrast.

## Format request intake

A new extension is not added to a picker merely because it is popular. Before another format is scheduled, Atlas records user value, open specification or maintainable parser route, DRM/legal boundary, security model, platform coverage, accessibility behavior, fixture provenance and the checkpoint that owns it. Until then it remains a candidate, not advertised support.
