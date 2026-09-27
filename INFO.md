# Atlas Reader Native — Current Project Notes

<!-- atlas-status: N3|accepted -->

N0–N3 are Accepted. The owner recorded explicit `N3 PASS` on 2026-09-27. Current version: `2.0.0-beta.1`.

Active branch: `native-v2-n3-library-index-foundation`. Native C++/Qt is active; Flutter is stopped and retained on `legacy/flutter` for backup and migration reference.

## Accepted Library foundation

SQLite/FTS5, explicit multi-root PDF scanning, guarded identity/reconciliation, background database work, list/grid Library, folders, Favorites, Recents and metadata search exist. Primary names use filename stems; document title/author/type are secondary. Default ordering follows filenames. N3 opens available PDFs in the system application; the Atlas reader begins in N4.

## Acceptance record

Checks 1–11 were exercised by the owner and their reported defects were corrected; a disposable Calibre 9.9 same-folder comparison closed check 12. See the [N3 acceptance record](docs/baselines/N3_ACCEPTANCE.md), [owner checklist](docs/N3_OWNER_TEST.md) and [evidence ledger](docs/baselines/N3_LIBRARY_INDEX_EVIDENCE.md).

## Windows 2.0 sequence

N4 PDF reader → N5 bookmarks → N6 ink/annotations → N7 utilities → F1 EPUB → F2 CBZ/CBR → F3 user-owned, DRM-free AZW3, MOBI and PRC files → F4 KFX decision → E1 pages → E2 text/images/equations → E3 forms → E4 redaction → E5 offline signatures → N8 mixed-format backup/migration → N9 Arabic/accessibility → N10 RC → N11 stable.

The [format roadmap](docs/FORMAT_EXPANSION_ROADMAP.md) and [competitive action register](docs/COMPETITIVE_GAP_ACTIONS.md) define implementation and tests. Open-source dependencies, offline reading and source preservation remain requirements. KFX feasibility alone cannot claim support.

## Frozen foundation

Accepted Qt PDF 6.10.3 handles PDF read/render/text/navigation; qpdf 12.4.1 handles qualified structural/security/write work. PDFium remains qualification-only. N2 acceptance/ADR-0004 are unchanged.

[CHECKPOINTS](CHECKPOINTS.md) owns order/status; [PLAN](PLAN.md) owns direction. [Scope](docs/FEATURE_SCOPE_2_0.md), [QA](docs/CHECKPOINT_QA_MATRIX.md), [release strategy](docs/RELEASE_STRATEGY.md) and [Git workflow](docs/GIT_WORKFLOW.md) govern delivery. Code and measured evidence determine implementation claims.
