# Atlas Reader Native — Agent Rules

Read this before changing the repository.

## Start with the current checkpoint

1. Read `CHECKPOINTS.md` and identify the active checkpoint.
2. Read only the relevant requirements/ADRs before editing.
3. Respect the checkpoint's explicit exclusions.
4. Do not begin work from a later checkpoint because it is convenient while touching the same files.

The Windows 2.0 release program is defined in `docs/RELEASE_STRATEGY.md`.

## Product priority

Work in the order **Index → Reader → Bookmarks** unless the active checkpoint explicitly requires another supporting slice. Do not expand scope because the native stack makes an unrelated feature attractive.

Windows 2.0 scope is defined in `docs/FEATURE_SCOPE_2_0.md`; P0 scope cannot be silently removed.

## Architecture rules

Read `docs/ARCHITECTURE.md` before changing cross-layer structure.

- Shared domain/core code is C++23 and platform-neutral.
- Do not introduce Win32, QML, Java/Kotlin, Objective-C/Swift, OS handles/URIs, or GUI types into core domain contracts.
- QML is presentation. Do not implement reconciliation, PDF security logic, identity matching, database rules, or save policy in QML/JavaScript.
- Platform behavior goes behind adapters.
- PDF behavior goes behind `IPdfEngine`/Atlas-owned facades. Never bind application logic directly to one PDF vendor API.
- Database access goes behind repositories/services; UI code never emits arbitrary SQL.
- Do not add a production dependency without applying `docs/DEPENDENCIES_AND_TOOLS.md` and `docs/LICENSING.md`.
- Persisted format/schema/architecture decisions that materially constrain the future require an ADR in `docs/decisions/`.

## Performance rules

Before changing hot/background/document work, read `docs/PERFORMANCE.md`.

Never synchronously perform directory traversal, PDF parse/render/save, hashing, cover generation, large SQL work, import/export, or large reconciliation on the UI/render thread.

Do not claim “zero lag.” Measure named scenarios and publish startup/frame/search/render/ink metrics using Release builds.

Use profiling tools (QML Profiler, Tracy, RenderDoc, WPA/WPR, VS profiler) to find measured bottlenecks rather than speculative rewrites.

## Document safety rules

Before touching save/security/bookmark behavior, read `docs/CORE_WORKFLOWS.md` and `docs/QUALITY_AND_TESTING.md`.

- Never bypass PDF security/password restrictions.
- Never silently mutate a signed/certified original when doing so may affect integrity.
- Never blindly overwrite a document that changed outside Atlas.
- Never report local work as embedded before validated commit/reopen.
- An unavailable library root is not evidence that all its books were deleted.
- Ambiguous document identity must not steal another copy's research data.
- Mutation tests use copied fixtures, never irreplaceable personal originals.

## Accessibility/i18n rules

Read `docs/UX_ACCESSIBILITY_AND_DESIGN.md` for user-facing work.

Preserve Arabic/RTL, mixed-script text, keyboard access, Narrator/UIA semantics, visible focus, 200% scaling, high contrast/system contrast, reduced-motion expectations, and non-color-only state.

N9 is final qualification, not permission to postpone accessibility/Arabic implementation.

## Quality/testing rules

Every behavioral change needs the smallest relevant evidence layer from `docs/QUALITY_AND_TESTING.md`:

- domain/unit;
- component/adapter;
- integration/fixture;
- preservation/interoperability;
- benchmark for hot path;
- manual hardware/a11y only where automation cannot prove it.

A defect found in beta/owner testing should gain a regression test or documented manual checklist entry.

## Documentation/status rules

Keep README, PLAN, CHECKPOINTS, INFO, CHANGELOG, relevant docs, and ADRs consistent.

Use these meanings precisely:

- **Planned** — documented requirement only.
- **Implemented** — code exists.
- **Verified** — automated/manual evidence exists.
- **Accepted** — owner explicitly passed the checkpoint.
- **Released** — accepted artifact published under version/tag.

Never upgrade a status merely because code compiles.

## Dependency and tooling rule

Do not add libraries, IDE plugins, cloud services, GitHub apps, telemetry, or external review tools “because they are useful.” First show the problem they solve and why existing Qt/C++/GitHub tooling does not solve it sufficiently.

Non-Qt native production dependencies move toward pinned vcpkg manifest mode from N2. Qt remains separately pinned/installed.

Development-only profilers/analyzers must not accidentally become runtime dependencies.

## CodeGraph / repository navigation

When a `.codegraph/` index exists locally, use CodeGraph for symbol/caller/impact navigation before repeated blind grep/search. The database is machine-local and ignored.

Project-local `.codex/config.toml` also defines a read-only GitHub MCP endpoint. Keep `GITHUB_PAT_TOKEN` in the user environment/secret store; never commit credentials.

## AI-generated code

AI/agent code has no reduced quality bar. Accept only when it:

- belongs to current checkpoint;
- follows architecture/dependency rules;
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

**N0 is bootstrap only.** Do not implement PDF, SQLite, scanner, reader, or bookmark product features until N0 is Accepted and N1 begins. N0 itself may change documentation, build/CI/tooling, minimal shell, core interfaces, and smoke tests needed to prove the foundation.