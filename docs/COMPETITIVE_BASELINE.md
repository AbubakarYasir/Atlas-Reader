# Atlas Reader Native — Competitive Baseline

## Purpose

Atlas competes for user attention with mature PDF/document products, but the goal is not feature-count parity. Competitor behavior is used to identify user expectations, interoperability requirements, and quality bars. Atlas should deliberately exceed them in its chosen core workflows rather than imitate every feature.

Primary references:

- Adobe Acrobat / Acrobat Reader
- Foxit PDF Editor / Reader
- Librera Reader
- Xournal++
- SumatraPDF
- Okular

The exact feature sets of competitors change over time. This document records **capability categories**, not permanent claims about who supports a specific checkbox.

## 1. Areas where mature products set expectations

### Adobe Acrobat

Reference for:

- PDF security/password distinction;
- signatures/certification consequences;
- standard PDF annotation interoperability;
- printing breadth;
- document metadata/security inspection;
- behavior on unusual enterprise PDFs.

Atlas response: match the safety/interoperability expectations relevant to our workflows, but do not chase office-suite/conversion/form/signing breadth for Windows 2.0.

### Foxit

Reference for:

- desktop-reader responsiveness;
- complete bookmark/outline tree operations;
- bookmark import/export patterns;
- PDF editing/security UX;
- tabbed desktop workflows and context actions.

Atlas response: bookmark management should feel complete, but Atlas adds first-class local fallback for documents that cannot safely be changed.

### Librera Reader

Reference for:

- reader-first library experience;
- broad reading preferences;
- mobile-friendly navigation;
- local/offline usage;
- save-as/new-output behaviors.

Atlas response: build a strong research library/reader while keeping desktop Windows quality first and avoiding clutter.

### Xournal++

Reference for:

- responsive pen-first interaction;
- non-destructive local/journal layer concept;
- useful annotation workflow separated from background PDF.

Atlas response: live ink must be independent of PDF serialization latency, and local overlays are valid product state rather than a failed-save cache.

### SumatraPDF

Reference for:

- fast startup;
- low-friction Windows reading;
- minimal, responsive interface;
- efficient common document navigation.

Atlas response: advanced research features do not excuse a slow/heavy baseline. Empty-shell and simple-reader performance is measured from N1 onward.

### Okular

Reference for:

- open-source document viewing;
- multi-format/document-navigation patterns;
- annotation/navigation integration;
- Linux accessibility/platform expectations later.

Atlas response: maintain open architecture and portable core without forcing Windows compromises to support future Linux.

## 2. Atlas differentiation

Atlas should become recognizably better at this combination:

1. **Library + exact research navigation as one product** — not merely a file-open PDF viewer.
2. **Deep bookmarks as a primary research object** — not a narrow sidebar feature.
3. **Portable when possible, local when necessary** — protected/read-only/signed/offline documents remain research-usable.
4. **Local-first/account-free** — no cloud service required for core value.
5. **Arabic/RTL/mixed-script correctness** — architecture and tests, not a translated skin.
6. **Accessibility built into custom reader/tree controls**.
7. **Explicit conflict/recovery behavior** — no silent overwrite or false save status.
8. **Performance budgets** — measurable responsiveness under large books/libraries.

## 3. Windows 2.0 parity bar by workflow

### Library/index

Atlas 2.0 must provide a more research-oriented library than ordinary PDF editors: multiple roots, progressive indexing, search, favorites/recents, guarded identity, offline-state preservation, direct-open documents, and global bookmark search.

### Reader

Atlas must meet the baseline users expect from a serious desktop reader: fast open, smooth scroll/zoom, accurate page navigation, tabs/session handling, text search where available, links, thumbnails/outlines, and keyboard operation.

### Bookmarks

This is a strategic area where Atlas should aim beyond “baseline”:

- complete tree editing;
- exact breadcrumbs/destinations;
- duplicate names safely handled;
- global search;
- local overlay on non-writable PDFs;
- reconciliation when external changes happen;
- lossless Atlas export plus readable Markdown/CSV.

### Ink/annotation

Atlas should match the common pen/highlight/note workflow sufficiently for research, but does not need every diagram/prepress editing tool in Windows 2.0.

### Printing

Meet normal serious reader expectations and preserve source integrity; do not turn N7 into a prepress suite.

## 4. Non-goals in the competitive race

We do not “lose” because a giant suite has features outside our thesis.

Windows 2.0 does not compete on:

- OCR;
- document conversion;
- arbitrary content editing;
- forms authoring;
- e-sign workflow platforms;
- enterprise cloud review;
- AI assistants;
- multimedia;
- full office-suite functionality.

Adding those before the core is excellent would make Atlas weaker, not stronger.

## 5. How competitor research is performed

At the start of each relevant checkpoint:

1. Re-check current official documentation/releases for the workflow.
2. Test current versions personally where available and legal.
3. Capture the user problem/pattern, not visual cloning.
4. Record useful behavior and failure states in the checkpoint notes/ADR if it changes our design.
5. Do not copy proprietary assets/code/UI verbatim.

## 6. Competitive scorecard for owner review

At major beta milestones, review Atlas against this qualitative matrix without turning it into a vanity score:

| Area | Question |
|---|---|
| Startup | Does Atlas feel immediate on ordinary hardware? |
| Open/render | Can a large PDF open/navigate without blocking the shell? |
| Library | Can a serious local collection be found/search/recovered more easily than in file-open editors? |
| Bookmarks | Is deep outline work clearly a first-class capability? |
| Safety | Are restricted/signed/conflicted/offline states handled more transparently than generic “save failed”? |
| Interop | Does standard portable PDF data survive and open correctly elsewhere? |
| Pen | Does ink track input without waiting on PDF writes? |
| Arabic/RTL | Is mixed-script research comfortable rather than merely technically supported? |
| Accessibility | Can core workflows be completed with keyboard/Narrator? |
| Recovery | Can the user understand where data lives and recover/export it? |
| Clutter | Do advanced capabilities remain discoverable without turning the reader into a toolbar wall? |

## 7. Rule

When a competitor feature request appears, ask:

> Does this materially strengthen Index, Reader, Bookmarks, data safety, interoperability, accessibility, or the accepted Windows 2.0 workflow?

If not, place it in the backlog/deferred list rather than disrupting the active checkpoint.