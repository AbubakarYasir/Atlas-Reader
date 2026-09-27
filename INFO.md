# Atlas Reader Native — Current Project Notes

<!-- atlas-status: N3|ready-for-owner-test -->

N0–N2 are Accepted. N3 Library/index foundation is Ready for owner test, not Accepted. Current version: `2.0.0-alpha.2`; beta.1 promotion requires the complete owner checklist and explicit `N3 PASS`.

Active branch: `native-v2-n3-library-index-foundation`. Native C++/Qt is active; Flutter is stopped and retained on `legacy/flutter` for backup and migration reference.

## Implemented candidate

SQLite/FTS5, explicit multi-root PDF scanning, guarded identity/reconciliation, background database work, list/grid Library, folders, Favorites, Recents and metadata search exist. Primary names use filename stems; document title/author/type are secondary. Default ordering follows filenames. N3 opens available PDFs in the system application; the Atlas reader begins in N4.

## Remaining acceptance

Use the [owner checklist](docs/N3_OWNER_TEST.md) and [evidence ledger](docs/baselines/N3_LIBRARY_INDEX_EVIDENCE.md). The latest metadata correction needs retesting; the skipped comparison remains open. Earlier numbered owner PASS reports remain evidence but do not constitute final N3 acceptance.

## Windows 2.0 sequence

N4 PDF reader → N5 bookmarks → N6 ink/annotations → N7 utilities → F1 EPUB → F2 CBZ/CBR → F3 user-owned, DRM-free AZW3, MOBI and PRC files → F4 KFX decision → E1 pages → E2 text/images/equations → E3 forms → E4 redaction → E5 offline signatures → N8 mixed-format backup/migration → N9 Arabic/accessibility → N10 RC → N11 stable.

The [format roadmap](docs/FORMAT_EXPANSION_ROADMAP.md) and [competitive action register](docs/COMPETITIVE_GAP_ACTIONS.md) define implementation and tests. Open-source dependencies, offline reading and source preservation remain requirements. KFX feasibility alone cannot claim support.

## Frozen foundation

Accepted Qt PDF 6.10.3 handles PDF read/render/text/navigation; qpdf 12.4.1 handles qualified structural/security/write work. PDFium remains qualification-only. N2 acceptance/ADR-0004 are unchanged.

[CHECKPOINTS](CHECKPOINTS.md) owns order/status; [PLAN](PLAN.md) owns direction. [Scope](docs/FEATURE_SCOPE_2_0.md), [QA](docs/CHECKPOINT_QA_MATRIX.md), [release strategy](docs/RELEASE_STRATEGY.md) and [Git workflow](docs/GIT_WORKFLOW.md) govern delivery. Code and measured evidence determine implementation claims.
