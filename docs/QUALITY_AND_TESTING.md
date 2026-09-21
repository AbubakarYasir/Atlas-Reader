# Atlas Reader Native — Quality, Testing, and Evidence Gates

## Principle

Atlas is document/research software. Correctness means more than “does not crash.” A release can be fast and still be unacceptable if it corrupts a PDF, attaches bookmarks to the wrong copy, loses Arabic text, breaks accessibility, or silently discards local research.

Every checkpoint therefore produces evidence across correctness, preservation, performance, accessibility, and recovery.

## 1. Test layers

### Domain/unit tests

Pure C++ tests without a GUI or real user profile.

Cover:

- document identity decisions;
- capability state transitions;
- bookmark tree operations;
- overlay state transitions;
- reconciliation/diff planning;
- page/index/label conversion;
- scan reconciliation rules;
- migration mapping;
- backup schema/version rules;
- failure/result typing.

### Component tests

Test one infrastructure adapter against controlled temporary resources:

- SQLite repositories/migrations/FTS;
- filesystem enumeration/watch;
- task scheduler/cancellation;
- PDF engine adapter;
- serializer/import/export;
- backup/restore.

### Contract tests

Every implementation of an interface must satisfy shared expectations. Example: Qt PDF/PDFium adapters must normalize page count/rotation/boxes/destinations consistently enough for application code.

### Integration tests

Exercise real subsystem combinations:

- root → scan → SQLite → search;
- PDF open → render → navigate;
- bookmark overlay → reconcile → safe save → reopen;
- source changed externally → conflict;
- backup → clean profile → restore;
- Flutter migration → native profile.

### GUI tests

Use Qt Test/QML testing plus focused native automation where useful. Test behavior, focus, state, and semantics rather than brittle pixel positions.

### Manual owner/hardware tests

Required where automation is insufficient:

- pen/touch latency/behavior;
- Narrator workflow quality;
- physical printing;
- high-DPI/multi-monitor behavior;
- external drives/HDDs;
- visual Arabic/mixed-direction correctness;
- installer/upgrade/uninstall.

## 2. PDF fixture corpus

Never use irreplaceable originals for mutation tests. Maintain redistributable/synthetic fixtures plus local opt-in private copied fixtures.

Required categories by N5/N6:

- simple writable PDF;
- mixed Arabic/English outline;
- duplicate outline names under different parents;
- deeply nested outline;
- no outline;
- malformed outline but readable pages;
- image-only/scanned PDF;
- mixed page sizes;
- rotated pages;
- unusual crop/media boxes;
- page labels different from indices;
- open-password encrypted PDF;
- permission-restricted PDF;
- filesystem read-only copy;
- signed/certified test document;
- externally modified revision pair;
- file sharing-lock scenario;
- long PDF;
- very large outline (10,000+ nodes);
- metadata/XMP-rich PDF;
- existing annotations from another standard reader.

Fixtures must document expected results, license/provenance, whether mutation is permitted, and checksum.

## 3. Preservation assertions

Any PDF mutation test checks relevant invariants before/after:

- page count;
- page dimensions/rotation;
- existing outline hierarchy/order/destinations;
- unrelated existing annotations;
- document information;
- XMP presence/content as applicable;
- encryption/security policy;
- signatures/certification behavior;
- attachments/other structures when the chosen writer might affect them;
- visual/content stream stability where the operation should not alter pages.

Do not rely only on opening the output successfully.

## 4. Golden interoperability tests

For embedded portable data, reopen the output with:

1. Atlas through a fresh engine/session; and
2. at least one independent standard PDF implementation/tool where practical.

For critical outline/annotation cases, test artifacts should be inspectable in Acrobat/Foxit or another independent reader during owner qualification. Atlas must not validate itself only with the same code that wrote the file.

## 5. Database tests

- migrations from every supported previous schema;
- transaction rollback on failure;
- FTS update consistency;
- WAL/recovery behavior where configured;
- interrupted process/reopen scenarios;
- offline-root state retained;
- duplicate/rename identity behavior;
- no orphan local-overlay state after safe operations;
- backup/restore schema compatibility.

Migration tests use copied temporary profiles only.

## 6. Fuzzing and hostile input

Introduce fuzzing once parsers/serializers owned by Atlas become meaningful.

Priority targets:

- Atlas bookmark JSON import;
- backup manifest parsing;
- path/document identity inputs;
- bookmark hierarchy/depth/order operations;
- migration input validation.

Third-party PDF engines already face complex PDF input; Atlas still needs tests ensuring engine errors are contained and never become unsafe writes or UI crashes.

## 7. Static/build gates

By the first beta, CI should require:

- clean configure;
- Release build;
- unit/component/integration tests;
- clang-format check;
- curated clang-tidy set;
- high compiler warning level;
- no new warnings;
- dependency/license inventory;
- secrets scan;
- source/version metadata consistency.

Additional jobs as the codebase matures:

- clang-cl build;
- AddressSanitizer build where supported;
- Linux sanitizer job once Linux CI is introduced;
- CodeQL C/C++ analysis;
- coverage report for domain/core logic;
- release-package smoke install/run.

Coverage is a diagnostic, not a target to game. Critical rules require explicit tests even if aggregate percentage is high.

## 8. Performance gates

Performance regression tests are separate from functional correctness.

Rules:

- compare Release builds;
- record machine/CPU/RAM/storage/GPU/OS;
- warm/cold state declared;
- median and p95/p99 where appropriate;
- enough iterations to reduce noise;
- never accept a faster result that violates correctness/preservation.

See `PERFORMANCE.md` for budgets.

## 9. Accessibility gates

Every user-facing checkpoint verifies:

- complete keyboard route;
- logical tab/focus order;
- visible focus;
- UI Automation role/name/state for custom controls;
- Narrator reading/activation workflow;
- 100% and 200% text/UI scale;
- system high-contrast behavior;
- non-color-only states;
- Arabic direction and announcements;
- error/recovery dialogs usable without pointer.

Use Accessibility Insights for Windows as a diagnostic/automated aid; it does not replace manual Narrator tests.

## 10. Arabic/Unicode gates

Representative fixtures include:

- Arabic only;
- English only;
- Arabic + English mixed title;
- Urdu;
- Arabic numerals and Western numerals;
- punctuation/brackets;
- combining marks/diacritics;
- long Unicode paths;
- filename/path segments with both RTL and LTR text.

Round trips must preserve Unicode content exactly unless an explicitly documented normalization operation is intentional.

## 11. Data-safety failure injection

Before stable 2.0, simulate:

- source locked by another process;
- permission removed after open;
- disk full/insufficient space where practical;
- temporary file validation failure;
- source moved/deleted while open;
- source modified externally;
- removable root disconnected;
- application terminated during staged save at controlled test points;
- corrupted/partial local database copy;
- restore with unresolved/ambiguous PDFs.

The expected outcome is preservation/recoverability, not merely an error dialog.

## 12. Security/privacy gates

- passwords never appear in logs, DB, exports, crash bundles, or test snapshots;
- ordinary logs do not contain document text or bookmark descriptions;
- no network access required for core runtime;
- dependencies reviewed for known relevant security updates before RC;
- signed/certified PDFs never silently lose integrity claims;
- import formats validate sizes/depths to prevent memory/resource abuse;
- paths are not executed as commands;
- shell integration treats filenames as untrusted input.

## 13. PR/checkpoint evidence template

Each checkpoint handoff records:

```text
Checkpoint:
Version/build:
Commit:
Toolchain:
Dependencies changed:
Automated tests:
Focused fixtures:
Performance evidence:
Accessibility/RTL evidence:
Manual hardware evidence:
Known limitations:
Data migrations involved:
Artifacts/checksums:
Owner result:
```

A checkpoint cannot be Accepted with unexplained skipped core tests.

## 14. Regression ownership

Every defect that reaches owner testing or a published beta should produce a regression test when technically reasonable. If it cannot be automated, add it to the numbered manual acceptance checklist and document why.

The purpose of the checkpoint system is that Atlas gets harder to break as it grows.