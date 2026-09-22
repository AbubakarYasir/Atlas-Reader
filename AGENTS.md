# Atlas Reader Native — Agent Rules

Read this before changing the repository.

## Start with the current checkpoint

1. Read `CHECKPOINTS.md` and identify the active checkpoint.
2. Read the active checkpoint plan/evidence files. For N2 these are `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md` and `docs/baselines/N2_PDF_ENGINE_MATRIX.md`.
3. Read only the relevant requirements/ADRs before editing.
4. Respect the checkpoint's explicit exclusions.
5. Do not begin work from a later checkpoint because it is convenient while touching the same files.

The Windows 2.0 release program is defined in `docs/RELEASE_STRATEGY.md`.

## Checkpoint immutability

A passed checkpoint cannot be reopened without new evidence of a user-facing regression.

N0 and N1 are Accepted. N2 work may rely on their accepted contracts but must not retune or reinterpret the N1 shell/toolchain baseline merely because a new dependency experiment makes a different configuration convenient.

## Product priority

Work in the order **Index → Reader → Bookmarks** unless the active checkpoint explicitly requires another supporting slice. N2 is a deliberate pre-feature PDF-engine qualification checkpoint; it does not authorize building the production Reader early.

Windows 2.0 scope is defined in `docs/FEATURE_SCOPE_2_0.md`; P0 scope cannot be silently removed. Product success and regression priorities are defined in `docs/SUCCESS_METRICS.md`.

## Architecture rules

Read `docs/ARCHITECTURE.md` before changing cross-layer structure.

- Shared domain/core code is C++23 and platform-neutral.
- Do not introduce Win32, QML, Java/Kotlin, Objective-C/Swift, OS handles/URIs, or GUI types into core domain contracts.
- QML is presentation. Do not implement reconciliation, PDF security logic, identity matching, database rules, or save policy in QML/JavaScript.
- Platform behavior goes behind adapters.
- PDF behavior goes behind `IPdfEngine`/Atlas-owned facades. Never bind application logic directly to one PDF vendor API.
- N2 may split PDF responsibilities across read/render and structural adapters; engine-native types must not cross the Atlas-owned contract boundary.
- Database access goes behind repositories/services; UI code never emits arbitrary SQL.
- Do not add a production dependency without applying `docs/DEPENDENCIES_AND_TOOLS.md` and `docs/LICENSING.md`.
- Persisted format/schema/architecture decisions that materially constrain the future require an ADR in `docs/decisions/`.

## N2 PDF-engine rules

Before PDF-engine work, read:

- `docs/N2_PDF_ENGINE_QUALIFICATION_PLAN.md`;
- `docs/baselines/N2_PDF_ENGINE_MATRIX.md`;
- `docs/decisions/ADR-0004-pdf-engine-responsibilities.md`;
- `tests/fixtures/pdf/README.md`;
- `docs/QUALITY_AND_TESTING.md`;
- `docs/LICENSING.md`.

N2 rules:

- no candidate is pre-selected;
- ADR-0004 remains **Proposed** until explicit owner `N2 PASS`;
- use the same normalized fixture expectations when comparing Qt PDF and PDFium;
- qpdf is primarily a structure/security/transformation candidate, not a raster engine;
- PDFium's upstream non-thread-safe API contract must be respected; do not invent concurrent calls to make a benchmark look better;
- if a community PDFium binary distribution is used for a spike, pin the exact package/revision and SHA-256 and record that it is not Google-official binary provenance;
- record exact qpdf release/vcpkg version rather than collapsing differing upstream version surfaces;
- mutation experiments use disposable fixture copies, never user originals;
- do not add SQLite/FTS5, production reader UI, bookmarks editor, annotations, migration, or installer work in N2.

## Data-format rules

Before changing document identity, bookmarks, destinations, SQLite persistence, import/export, backup, migration, or local overlays, read `docs/DATA_MODEL_AND_FORMATS.md`.

- Engines/databases are implementations; Atlas-owned data semantics stay engine-independent.
- Do not identify documents by filename/path alone.
- Do not treat FTS/cache structures as durable research data.
- Do not introduce an unversioned Atlas-owned persisted/interchange format.
- Do not silently downgrade or discard unknown/newer durable data.
- A bookmark is not merely title + page number; preserve stable identity, hierarchy/order, destination semantics, and storage state.

## Performance rules

Before changing hot/background/document work, read `docs/PERFORMANCE.md`.

Never synchronously perform directory traversal, PDF parse/render/save, hashing, cover generation, large SQL work, import/export, or large reconciliation on the UI/render thread.

Do not claim “zero lag.” Measure named scenarios and publish startup/frame/search/render/ink metrics using Release builds.

For N2, rendering/open/text/search comparisons must use the same fixture/work request, state warm/cold context, report median/p95 where applicable, and never prefer a faster incorrect result.

Use profiling tools (QML Profiler, Tracy, RenderDoc, WPA/WPR, VS profiler) to find measured bottlenecks rather than speculative rewrites.

## Document safety rules

Before touching save/security/bookmark behavior, read `docs/CORE_WORKFLOWS.md`, `docs/SECURITY_MODEL.md`, and `docs/QUALITY_AND_TESTING.md`.

- Never bypass PDF security/password restrictions.
- Never silently mutate a signed/certified original when doing so may affect integrity.
- Never blindly overwrite a document that changed outside Atlas.
- Never report local work as embedded before validated commit/reopen.
- An unavailable library root is not evidence that all its books were deleted.
- Ambiguous document identity must not steal another copy's research data.
- Mutation tests use copied fixtures, never irreplaceable personal originals.

## Fixture/privacy rules

Committed PDF fixtures require documented provenance, redistribution permission and SHA-256. Private owner PDFs may supplement qualification locally, but their bytes, personal paths, titles, extracted text and passwords do not belong in public Git history or ordinary CI logs.

Test passwords must be synthetic and have no value outside the fixture.

Do not make one candidate engine the sole oracle for expected output from another engine.

## Accessibility/i18n rules

Read `docs/UX_ACCESSIBILITY_AND_DESIGN.md` for user-facing work.

Preserve Arabic/RTL, mixed-script text, keyboard access, Narrator/UIA semantics, visible focus, 200% scaling, high contrast/system contrast, reduced-motion expectations, and non-color-only state.

N2 engine qualification must include Arabic/Unicode text/search/outline fixtures because N9 is final qualification, not permission to postpone Arabic correctness.

## Quality/testing rules

Every behavioral change needs the smallest relevant evidence layer from `docs/QUALITY_AND_TESTING.md`:

- domain/unit;
- component/adapter;
- integration/fixture;
- preservation/interoperability;
- benchmark for hot path;
- manual hardware/a11y only where automation cannot prove it.

A defect found in beta/owner testing should gain a regression test or documented manual checklist entry.

Use `docs/RISK_REGISTER.md` when a change creates or changes a significant architecture, data, licensing, platform, security, or delivery risk.

## Documentation/status rules

Keep README, PLAN, CHECKPOINTS, INFO, CHANGELOG, `docs/README.md`, relevant specialized docs, and ADRs consistent.

Every meaningful N2 experiment updates the durable matrix with exact engine version/revision, fixture IDs, evidence, limitation/failure and decision impact.

Use these meanings precisely:

- **Planned** — documented requirement only.
- **Implemented** — code exists.
- **Verified** — automated/manual evidence exists.
- **Accepted** — owner explicitly passed the checkpoint.
- **Released** — accepted artifact published under version/tag.

Never upgrade a status merely because code compiles.

## Dependency and tooling rule

Do not add libraries, IDE plugins, cloud services, GitHub apps, telemetry, or external review tools “because they are useful.” First show the problem they solve and why existing Qt/C++/GitHub tooling does not solve it sufficiently.

Use `docs/UPSTREAM_CATALOG.md` as the operational candidate list. Non-Qt native production dependencies move toward pinned vcpkg manifest mode from N2. Qt remains separately pinned/installed.

Development-only profilers/analyzers must not accidentally become runtime dependencies.

## CodeGraph / repository navigation

When a `.codegraph/` index exists locally, use CodeGraph for symbol/caller/impact navigation before repeated blind grep/search. The database is machine-local and ignored.

Project-local `.codex/config.toml` also defines a read-only GitHub MCP endpoint. Keep `GITHUB_PAT_TOKEN` in the user environment/secret store; never commit credentials.

## AI-generated code

AI/agent code has no reduced quality bar. Accept only when it:

- belongs to current checkpoint;
- follows architecture/dependency/data-format rules;
- compiles;
- passes relevant tests/static analysis;
- includes tests for new behavior;
- updates docs/contracts when needed;
- contains no secrets/incompatible copied code.

Do not merge broad speculative refactors just because an agent proposes them.

## Migration rule

The Flutter implementation is a behavioral/data reference. Do not copy its widget architecture or reproduce known performance problems mechanically. Port contracts, fixtures, and user-visible behavior deliberately.

Legacy profile migration is read-only from the source and must never destructively modify the old database.

## Secrets

Never inspect, print, commit, or request secrets unnecessarily. Password-protected PDF fixture passwords must be test-only values, never personal document passwords.

## Current stop gate

**N0 Accepted. N1 Accepted. N2 is active and In progress.**

N2 may add only the contracts, fixtures, probe/adaptor code, dependency/bootstrap configuration, benchmarks, preservation/security experiments, licensing evidence and documentation required to qualify Qt PDF, PDFium and qpdf and assign responsibilities.

Do **not** build the production Reader, SQLite/FTS5 index, scanner, bookmark editor/local overlay, annotations/ink, migration, installer, or later-checkpoint feature code.

N2 is not Accepted until the evidence matrix and ADR-0004 are complete, strict final CI passes, and the owner explicitly records `N2 PASS`. N3 remains closed until then.
