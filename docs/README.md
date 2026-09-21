# Atlas Reader Native — Documentation Index

Use this page to find the authoritative document instead of duplicating requirements across random files.

## Start here

1. **`../README.md`** — project entry point and current status.
2. **`../PLAN.md`** — master product/engineering plan.
3. **`../CHECKPOINTS.md`** — current checkpoint, release mapping, stop gates.
4. **`FEATURE_SCOPE_2_0.md`** — binding Windows 2.0 feature scope.
5. **`SUCCESS_METRICS.md`** — how we prove the rewrite is becoming a better product rather than merely larger.

## Product behavior

- **`CORE_WORKFLOWS.md`** — Library/Index → Reader → Bookmarks capability, local fallback, security, conflict, import/export, recovery.
- **`DATA_MODEL_AND_FORMATS.md`** — portable vs local authority, document identity, bookmark states, destinations, Atlas JSON/backup/migration contracts.
- **`UX_ACCESSIBILITY_AND_DESIGN.md`** — interaction model, keyboard, Arabic/RTL, Narrator, scaling, state communication.
- **`COMPETITIVE_BASELINE.md`** — what we learn from Acrobat/Foxit/Librera/Xournal++/Sumatra/Okular and what Atlas intentionally does differently.

## Engineering

- **`ARCHITECTURE.md`** — layers, thread model, reader viewport, PDF/storage/platform ports.
- **`TECH_STACK.md`** — chosen technical stack and alternatives.
- **`DEPENDENCIES_AND_TOOLS.md`** — open-source dependencies, profilers, IDE plugins, GitHub/agent automation, acceptance policy.
- **`UPSTREAM_CATALOG.md`** — concrete upstream repos/tools and the checkpoint at which each may be adopted.
- **`SECURITY_MODEL.md`** — untrusted PDFs/imports/paths, password/privacy, safe mutation, dependency security, hardening.
- **`PERFORMANCE.md`** — budgets, benchmark methodology, regression rules.
- **`QUALITY_AND_TESTING.md`** — unit/component/integration/fixture/interoperability/accessibility/failure-injection gates.
- **`RISK_REGISTER.md`** — current architectural/product/release risks and required mitigations.
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
- **`../SECURITY.md`** — vulnerability/security reporting and repository security expectations.

## Authority rule

When two documents appear to conflict:

1. Running code/tests define what is actually Implemented.
2. `CHECKPOINTS.md` defines current execution/status.
3. `FEATURE_SCOPE_2_0.md` defines accepted product scope.
4. `PLAN.md` defines principles/north star.
5. `DATA_MODEL_AND_FORMATS.md` defines Atlas-owned persisted/interchange semantics.
6. Specialized docs define their domain contract.
7. Older prose must be corrected rather than used to justify contradictory behavior.

Never infer that a Planned requirement is already shipped.

## Change discipline

Architecture, P0 scope, durable data formats, release gates, or dependency strategy must not change silently. Update the relevant specialized document, add/update an ADR when the decision is architectural, revise checkpoint evidence, and note material changes in `CHANGELOG.md`.