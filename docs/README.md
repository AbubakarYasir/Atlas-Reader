# Atlas Reader Native — Documentation Index

Use this page to find the authoritative document instead of duplicating requirements across random files.

## Start here

1. **`../README.md`** — project entry point and current status.
2. **`../PLAN.md`** — master product/engineering plan.
3. **`../CHECKPOINTS.md`** — current checkpoint, release mapping, stop gates.
4. **`FEATURE_SCOPE_2_0.md`** — binding Windows 2.0 feature scope.

## Product behavior

- **`CORE_WORKFLOWS.md`** — Library/Index → Reader → Bookmarks capability, local fallback, security, conflict, import/export, recovery.
- **`UX_ACCESSIBILITY_AND_DESIGN.md`** — interaction model, keyboard, Arabic/RTL, Narrator, scaling, state communication.
- **`COMPETITIVE_BASELINE.md`** — what we learn from Acrobat/Foxit/Librera/Xournal++/Sumatra/Okular and what Atlas intentionally does differently.

## Engineering

- **`ARCHITECTURE.md`** — layers, thread model, reader viewport, PDF/storage/platform ports.
- **`TECH_STACK.md`** — chosen technical stack and alternatives.
- **`DEPENDENCIES_AND_TOOLS.md`** — open-source dependencies, profilers, IDE plugins, GitHub/agent automation, acceptance policy.
- **`SECURITY_MODEL.md`** — untrusted PDFs/imports/paths, password/privacy, safe mutation, dependency security, hardening.
- **`PERFORMANCE.md`** — budgets, benchmark methodology, regression rules.
- **`QUALITY_AND_TESTING.md`** — unit/component/integration/fixture/interoperability/accessibility/failure-injection gates.
- **`BUILDING.md`** — local/CI build instructions and toolchain pins.
- **`LICENSING.md`** — third-party license/distribution/SBOM requirements.

## Delivery

- **`RELEASE_STRATEGY.md`** — `2.0.0-alpha` → beta → RC → stable rules.
- **`DEVELOPMENT_WORKFLOW.md`** — branches, PRs, commits, ADRs, AI/agent workflow, CI tiers.
- **`PLATFORM_ROADMAP.md`** — Windows → Android → Linux → macOS → iOS/iPadOS.
- **`MIGRATION_FROM_FLUTTER.md`** — side-by-side behavior and legacy profile migration policy.
- **`decisions/`** — architecture decision records.

## Status/history

- **`../INFO.md`** — compact current orientation/status.
- **`../CHANGELOG.md`** — repository changes.
- **`../AGENTS.md`** — mandatory rules for coding agents and automated contributors.

## Authority rule

When two documents appear to conflict:

1. Running code/tests define what is actually Implemented.
2. `CHECKPOINTS.md` defines current execution/status.
3. `FEATURE_SCOPE_2_0.md` defines accepted product scope.
4. `PLAN.md` defines principles/north star.
5. Specialized docs define their domain contract.
6. Older prose must be corrected rather than used to justify contradictory behavior.

Never infer that a Planned requirement is already shipped.