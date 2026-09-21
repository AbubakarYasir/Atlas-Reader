# Atlas Reader Native — Development Workflow

## Goal

Keep development fast without losing architectural discipline. Atlas is built checkpoint-by-checkpoint, not through a long-lived feature branch that becomes impossible to review.

## 1. Branch model

- `main` in the eventual native repository: accepted checkpoint state only.
- `checkpoint/N#-short-name`: active checkpoint integration branch if needed.
- short-lived feature/fix branches: one coherent change each.
- engine experiments: `spike/...` branches; never silently merged into production architecture.

Do not maintain permanent platform forks. Platform-specific code belongs behind ports/adapters in the same repository.

## 2. Checkpoint workflow

For every checkpoint:

1. Read `PLAN.md`, `CHECKPOINTS.md`, relevant feature/architecture docs, and current ADRs.
2. Restate scope and explicit exclusions in the branch/issue.
3. Add/adjust tests and benchmark fixtures before or with implementation.
4. Implement the smallest architectural slice that satisfies the checkpoint.
5. Run local format/build/test/static-analysis gates.
6. Run focused PDF/DB/platform fixtures.
7. Record performance/accessibility/manual evidence required by that checkpoint.
8. Update docs/status/changelog.
9. Hand off for owner test.
10. Stop. Do not begin the next checkpoint until accepted.

### Accepted-checkpoint immutability

A passed checkpoint cannot be reopened without **new evidence of a user-facing regression** in the accepted state.

- New preferences, theoretical risks, retrospective stricter criteria, or unrelated toolchain churn do not invalidate an accepted checkpoint.
- Such concerns belong to the active/later checkpoint, release qualification, or backlog unless they produce concrete regression evidence against the accepted behavior.
- If new user-facing regression evidence does require reopening a passed checkpoint, record the evidence, the affected acceptance item, and the corrective scope explicitly before changing its status.

This rule prevents already-proven work from becoming permanently provisional while still allowing real regressions to be corrected.

## 3. Commit policy

Prefer reviewable commits with intent:

- `build:` build/toolchain changes
- `core:` portable domain/application logic
- `pdf:` PDF engine/adapters
- `index:` library/SQLite/search
- `reader:` rendering/navigation
- `bookmarks:` outline/overlay/reconciliation
- `ink:` writing/annotations
- `a11y:` accessibility
- `i18n:` localization/RTL
- `platform:` OS adapter work
- `test:` tests/fixtures/benchmarks
- `docs:` documentation/ADRs
- `fix:` contained defect fix

Avoid giant commits mixing dependency upgrades, architecture rewrites, UI redesign, and feature work.

## 4. Pull request requirements

A PR description includes:

- checkpoint;
- problem/requirement;
- implementation summary;
- architecture boundaries touched;
- dependencies changed;
- tests added/run;
- performance impact/evidence if hot path;
- data/schema/file-format changes;
- accessibility/RTL impact;
- screenshots/video only when useful;
- known limitations/follow-ups.

No PR should claim performance improvement without before/after evidence.

## 5. Architecture decision records

Create an ADR when changing one of these:

- GUI/framework/core language;
- PDF engine responsibility split;
- dependency manager;
- database/search model;
- persisted file/backup format;
- document identity algorithm;
- threading/task model;
- public extension/plugin architecture if ever added;
- installer/update technology;
- licensing/distribution strategy.

Small implementation choices do not need ADRs.

ADR states: Proposed → Accepted → Superseded/Rejected.

## 6. Dependency changes

Dependency PRs are deliberate and checkpointed. Never auto-merge dependency bumps.

Required review:

- release notes/security reason;
- license changes;
- vcpkg baseline/lock change;
- compile/test result;
- relevant PDF/benchmark regression result;
- packaging/notices change.

## 7. Generated/AI-assisted code

Agents are accelerators, not authorities.

Before modifying code an agent must:

- read `AGENTS.md`;
- identify current checkpoint;
- inspect relevant ADRs/interfaces;
- prefer local CodeGraph/symbol navigation over blind repository-wide rewrites;
- avoid adding dependencies unless explicitly required;
- never expose secrets or inspect secret files unnecessarily.

Agent patch acceptance is identical to human patch acceptance: compile, tests, static analysis, architecture, docs, evidence.

Do not accept speculative refactors solely because an AI says they are cleaner/faster.

## 8. Repository knowledge tooling

### CodeGraph

Use project-local graph navigation for symbol relationships/callers/impact once the codebase is non-trivial. The generated database remains ignored/reproducible.

### GitHub connector/MCP

Use read-only access for PR/diff/CI/repository context by default. Mutation should be explicit and tied to the active task.

### Documentation

Documentation is part of the source of truth, but code/runtime tests are the source of truth for shipped behavior. If docs and implementation disagree, fix the docs and/or code before acceptance.

## 9. Local pre-push checklist

As the relevant tools are introduced:

```text
cmake configure succeeds
Release build succeeds
CTest passes
clang-format check passes
clang-tidy baseline passes
no unexpected warnings
focused fixture tests pass
benchmark smoke has no major regression
```

Do not require heavyweight hardware/manual suites on every local edit; those belong at checkpoint/release gates.

## 10. CI tiers

### Fast PR CI

- configure/build;
- core/unit tests;
- component tests;
- format/static checks;
- lightweight fixture tests.

### Full checkpoint CI

- broader PDF corpus;
- integration tests;
- migration/backup tests as applicable;
- benchmark smoke;
- release build/package dry run.

### Release CI

- clean reproducible build from tag;
- full tests;
- package;
- deploy Qt/runtime dependencies correctly;
- generate notices/SBOM;
- checksum;
- artifact smoke test;
- prerelease/release publication only after gate conditions.

## 11. Performance workflow

When a regression is suspected:

1. reproduce Release build;
2. make a minimal benchmark/scenario;
3. profile with QML Profiler/Tracy/WPA/RenderDoc as appropriate;
4. find measured bottleneck;
5. fix architecture/data flow;
6. record before/after result;
7. add regression threshold if stable enough.

Avoid premature low-level micro-optimization when the actual problem is eager rendering, unbounded tasks, synchronous I/O, or incorrect caching.

## 12. Issue/backlog policy

Every idea belongs to one of:

- active checkpoint;
- next checkpoint candidate;
- Windows 2.0 backlog;
- post-2.0/platform backlog;
- rejected/deferred.

Do not smuggle unrelated backlog work into an active checkpoint because “we are already touching this file.”

## 13. Definition of ready-to-work

A checkpoint is ready to begin only when:

- previous checkpoint Accepted;
- requirements/exit criteria are written;
- dependencies/architecture assumptions are decided enough for that work;
- fixture/evidence plan exists;
- no unresolved blocker makes the planned evidence impossible.

This prevents coding for weeks only to discover the success criteria afterward.
