# N4 Native Reader Foundation Plan

<!-- atlas-status: N4|in-progress -->

**Planned version:** `2.0.0-beta.2`

**Status:** In progress from verified accepted `main` commit `739508e3e698016e8a6a4cb8b87c2b4aa418d66d`.

## Outcome

N4 delivers an Atlas-owned, offline PDF reading workspace comfortable enough for daily use. It replaces N3's external-app Open route with accurate, responsive in-app reading while preserving the accepted Library and PDF-engine boundaries.

N4 is read-only. It does not edit outlines, annotations, page content or the source PDF. Bookmark mutation begins in N5; ink/annotations begin in N6.

## Product contract

- Open a Library PDF or direct local PDF into an Atlas tab without blocking the interface.
- Center the document on a quiet reading canvas with intentional gutters and a persistent, collapsible navigation panel on desktop; use a drawer/overlay only when width requires it.
- Keep common actions visible and understandable without a crowded ribbon: page, zoom/fit, search, navigation panel and reading mode.
- Virtualize pages. Render visible/near-visible work first, cancel obsolete work and enforce explicit CPU/GPU memory budgets.
- Preserve correct crop/media boxes, rotation, mixed page sizes, page labels, outlines, internal/external links and exact destinations.
- Provide exact-layout reading plus a separately labelled simplified text view only where extracted text and reading order are trustworthy. Scans receive an honest unavailable state; there is no OCR claim.
- Support single page, continuous, facing, cover-page and reading-direction modes; side-by-side panes have explicit active focus and share one resource budget.
- Keep session restoration opt-in and local. Active PDF content never gains network or process-launch permission silently.
- Maintain Arabic/Urdu joining, diacritics, bidi, RTL interface behavior and mixed-script search/navigation.
- Retain generic book/session/progress boundaries needed by later EPUB/comic/DRM-free ebook checkpoints; PDF geometry and rendering stay in PDF-specific adapters.

## Subgates

### N4.1 — Session and open model

Implement asynchronous direct/Library open, immutable read sessions, tab lifecycle, duplicate-open policy, clear password/unsupported/malformed/missing states, local opt-in restoration and Windows file sharing that does not unnecessarily lock a user's PDF.

Tests cover success/error transitions, cancellation, reopen, duplicate opens, tab switching/closing, stale revisions, offline paths, restart policy and clean resource release.

**Current implementation checkpoint (2026-09-30):** the Atlas-owned session state machine, duplicate/reopen/tab lifecycle, background direct/Library open, initial in-app status workspace and opt-in path-only restoration are implemented. N4.1 is still In progress until the remaining restart/restoration, cancellation/resource-release, Windows sharing, password-entry, full regression and rendered accessibility evidence is complete. Page rendering belongs to N4.2 and has not started.

### N4.2 — Virtual viewport and render scheduler

Define engine-independent page geometry and render requests, visible/near-visible priorities, cancellation tokens, scale buckets, texture ownership, cache keys, eviction and hard budgets. No eager full-document rasterization.

Tests cover mixed sizes, all rotations, crop/media boxes, fast jumps, resize/scale changes, obsolete-task cancellation, cache invalidation and a 2,000-page memory series.

### N4.3 — Reading navigation

Implement continuous/page navigation, zoom, actual size, fit page/width/height, page-label jump, academic page offset, thumbnails, scroll/keyboard/touchpad behavior and reading modes. History must make return navigation predictable.

Tests cover Fit/FitH/FitV normalization, bounds, labels, rotations, facing/cover rules, LTR/RTL progression, focus and 100%/200% layouts.

### N4.4 — Outline, text, search and links

Expose read-only outlines, selectable/copyable text, cancellable search and safe internal/external links through Atlas-owned models. De-duplicate engine rows semantically and normalize destinations with rotation/crop metadata.

Tests cover link-heavy fixtures, duplicate URI rows, rotated destinations, Arabic/Urdu/English and mixed-direction search, long-document p50/p95/p99 latency, image-only honesty and active-content containment.

### N4.5 — Production reading workspace

Compose the canvas, tab strip, collapsible panel, search results, outline/thumbnails, side-by-side panes, status/error states and shortcuts from shared Atlas primitives/tokens. Avoid a ribbon clone, generic AI dashboard, excessive cards or unexplained icons.

Rendered review covers English and Arabic/RTL, light/dark/high contrast, wide/narrow, 100%/200%, long names, overflow, visible focus, Narrator/UIA names and complete keyboard operation.

## Performance evidence

Record named hardware/build/fixture data for time to first readable page, page-jump feedback/render latency, sustained scroll frame pacing, search p50/p95/p99, cache hit/eviction, idle/active memory, 2,000-page behavior and repeated multi-tab open/switch/close soak. Averages alone do not pass N4.

## Interoperability and safety

- PDFs remain authoritative and unchanged throughout N4; source hashes before/after owner tests must match.
- Qt PDF 6.10.3 remains the accepted read/render/text/navigation engine behind Atlas interfaces. qpdf remains structural/security/write infrastructure and is not needed for ordinary rendering.
- Preserve page count, page boxes, rotation, outlines, metadata, existing annotations and Arabic/Urdu display as read evidence. N4 makes no lossless-write claim.
- Password handling never bypasses authorization or logs secrets. External links require a clear user action; embedded script, launch and network behavior is blocked by default.
- Runtime stays offline and free of cloud dependencies.

## Verification sequence

1. Unit/contract tests for each subgate before UI composition depends on it.
2. Local Debug and Release builds/tests with warnings enabled.
3. Targeted geometry, navigation, search, Arabic/Urdu, security and performance fixtures.
4. Rendered Windows review of the actual app; inspect code and screenshots/runtime behavior.
5. Exact-head GitHub Debug/Release plus inherited PDF regression workflows.
6. Packaged clean-profile smoke with runtime dependencies and source-hash check.
7. Plain-language owner checklist and one exact checksum-bound package.

## Owner test

Read image-only, corrected Arabic/Urdu, long, mixed-size/rotation and link-heavy PDFs. Exercise tabs, zoom/fit, labels/offsets, search, links, outlines, thumbnails, modes, side-by-side and restoration with mouse and keyboard. Confirm Narrator route, 200% layout, responsiveness, bounded memory, honest unavailable states and unchanged source hashes. Compare the same core tasks with relevant mature readers for missing workflow gaps, not copied visuals.

Final acceptance requires the exact words `N4 PASS` after every required defect is fixed and retested.

## Stop gate

N4 becomes Accepted only when the reader is comfortable enough for daily PDF reading, all blocking QA is green, the exact package is owner-tested, and explicit `N4 PASS` is recorded. N5 cannot begin earlier.
