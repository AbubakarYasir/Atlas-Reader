# Atlas Reader Native — Current Project Notes

<!-- atlas-status: N4|in-progress -->

N0–N3 are Accepted. N4 native reader foundation is In progress. Current source version: `2.0.0-beta.1`; planned N4 acceptance version: `2.0.0-beta.2`.

Active branch: `native-v2-n4-reader-foundation`, based exactly on verified accepted `main` merge `739508e3e698016e8a6a4cb8b87c2b4aa418d66d`. Native C++/Qt is active; Flutter is stopped and retained on `legacy/flutter` for backup and migration reference.

## Accepted Library foundation

SQLite/FTS5, explicit multi-root PDF scanning, guarded identity/reconciliation, background database work, list/grid Library, folders, Favorites, Recents and metadata search exist. Primary names use filename stems; document title/author/type are secondary. Default ordering follows filenames. N3 opens available PDFs in the system application; the Atlas reader begins in N4.

## Acceptance record

Checks 1–11 were exercised by the owner and their reported defects were corrected; a disposable Calibre 9.9 same-folder comparison closed check 12. See the [N3 acceptance record](docs/baselines/N3_ACCEPTANCE.md), [owner checklist](docs/N3_OWNER_TEST.md) and [evidence ledger](docs/baselines/N3_LIBRARY_INDEX_EVIDENCE.md).

## Active work

N4.1 defines asynchronous read-only PDF sessions, tabs, open/error/password states, local opt-in restoration and clean resource release. The [N4 plan](docs/N4_READER_FOUNDATION_PLAN.md) and [evidence ledger](docs/baselines/N4_READER_FOUNDATION_EVIDENCE.md) are binding. No N4 implementation or verification is claimed at branch opening.

## Windows 2.0 sequence

N4 PDF reader → N5 bookmarks → N6 ink/annotations → N7 utilities → F1 EPUB → F2 CBZ/CBR → F3 user-owned, DRM-free AZW3, MOBI and PRC files → F4 KFX decision → E1 pages → E2 text/images/equations → E3 forms → E4 redaction → E5 offline signatures → N8 mixed-format backup/migration → N9 Arabic/accessibility → N10 RC → N11 stable.

The [format roadmap](docs/FORMAT_EXPANSION_ROADMAP.md) and [competitive action register](docs/COMPETITIVE_GAP_ACTIONS.md) define implementation and tests. Open-source dependencies, offline reading and source preservation remain requirements. KFX feasibility alone cannot claim support.

## Frozen foundation

Accepted Qt PDF 6.10.3 handles PDF read/render/text/navigation; qpdf 12.4.1 handles qualified structural/security/write work. PDFium remains qualification-only. N2 acceptance/ADR-0004 are unchanged.

[CHECKPOINTS](CHECKPOINTS.md) owns order/status; [PLAN](PLAN.md) owns direction. [Scope](docs/FEATURE_SCOPE_2_0.md), [QA](docs/CHECKPOINT_QA_MATRIX.md), [release strategy](docs/RELEASE_STRATEGY.md) and [Git workflow](docs/GIT_WORKFLOW.md) govern delivery. Code and measured evidence determine implementation claims.
