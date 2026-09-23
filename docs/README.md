# Atlas Reader Native — Documentation Index

Use this page to find the authoritative document instead of duplicating requirements across random files.

## Start here

1. **`../README.md`** — project entry point and current status.
2. **`../PLAN.md`** — master product/engineering north star.
3. **`../CHECKPOINTS.md`** — current checkpoint, release mapping, stop gates.
4. **`N2_PDF_ENGINE_QUALIFICATION_PLAN.md`** — active N2 execution plan, engine responsibilities, fixtures, measurements, hard blockers and stop gate.
5. **`baselines/N2_PDF_ENGINE_MATRIX.md`** — binding N2 evidence matrix and remaining gates.
6. **`decisions/ADR-0004-pdf-engine-responsibilities.md`** — Proposed N2 responsibility split decision; not binding until owner N2 acceptance.
7. **`../tests/fixtures/pdf/README.md`** — N2 PDF fixture provenance/privacy/checksum/mutation contract.
8. **`FEATURE_SCOPE_2_0.md`** — binding Windows 2.0 feature scope.
9. **`SUCCESS_METRICS.md`** — how we prove the rewrite is becoming a better product rather than merely larger.

N0 and N1 are Accepted. N2 is active. A passed checkpoint cannot be reopened without new evidence of a user-facing regression.

## Product behavior

- **`CORE_WORKFLOWS.md`** — Library/Index → Reader → Bookmarks capability, local fallback, security, conflict, import/export, recovery.
- **`DATA_MODEL_AND_FORMATS.md`** — portable vs local authority, document identity, bookmark states, destinations, Atlas JSON/backup/migration contracts.
- **`UX_ACCESSIBILITY_AND_DESIGN.md`** — interaction model, keyboard, Arabic/RTL, Narrator, scaling, state communication.
- **`COMPETITIVE_BASELINE.md`** — what we learn from Acrobat/Foxit/Librera/Xournal++/Sumatra/Okular and what Atlas intentionally does differently.

## Engineering

- **`ARCHITECTURE.md`** — layers, thread model, reader viewport, PDF/storage/platform ports.
- **`TECH_STACK.md`** — chosen technical stack and alternatives.
- **`TOOLCHAIN.md`** — accepted N1 Windows alpha toolchain baseline and drift policy; N2 inherits it unless candidate-specific probe tooling is explicitly documented.
- **`DEPENDENCIES_AND_TOOLS.md`** — open-source dependencies, PDF candidate acquisition constraints, profilers, IDE plugins, GitHub/agent automation, acceptance policy.
- **`UPSTREAM_CATALOG.md`** — concrete upstream repos/tools, current N2 PDF integration notes, and the checkpoint at which each may be adopted.
- **`SECURITY_MODEL.md`** — untrusted PDFs/imports/paths, password/privacy, safe mutation, dependency security, hardening.
- **`PERFORMANCE.md`** — budgets, benchmark methodology, regression rules.
- **`QUALITY_AND_TESTING.md`** — unit/component/integration/fixture/interoperability/accessibility/failure-injection gates.
- **`RISK_REGISTER.md`** — current architectural/product/release risks and required mitigations.
- **`BUILDING.md`** — local/CI build instructions; candidate-specific N2 commands are added only when a probe is introduced and pinned.
- **`LICENSING.md`** — third-party license/distribution/SBOM requirements.

## N2 PDF-engine qualification

N2 intentionally treats PDF responsibilities separately rather than assuming one universal library.

- **Qt PDF** — read/render/text/search/link/outline candidate.
- **PDFium** — competing read/render/text/search/link/outline candidate; upstream non-thread-safe API and build/supply-chain costs are measured explicitly.
- **qpdf** — structure/security/transformation candidate.

The permitted final result is a split architecture behind Atlas-owned normalized contracts.

Primary N2 documents:

- **`N2_PDF_ENGINE_QUALIFICATION_PLAN.md`** — phases N2.0 through N2.6 and evidence methodology;
- **`baselines/N2_PDF_ENGINE_MATRIX.md`** — binding capability-by-capability results, canonical evidence ledger, scope questions and remaining gates;
- **`baselines/N2_PDFIUM_PROVENANCE.md`** — PDFium pin/acquisition/build constraints;
- **`baselines/N2_QPDF_BASELINE.md`** — qpdf structural/security/transformation evidence;
- **`baselines/N2_QPDF_PROVENANCE.md`** — exact qpdf package provenance;
- **`baselines/N2_RENDERING_FIDELITY.md`** — A011 1×/2× crop/rotation/annotation/background rendering evidence;
- **`baselines/N2_COORDINATE_NORMALIZATION.md`** — Atlas page-space contract, A003/A011 geometry and A012 `/XYZ` destination evidence;
- **`baselines/N2_STRESS_CONCURRENCY.md`** — 500-lifetime read-engine stress and 1,000-job serialized PDFium queue evidence;
- **`baselines/N2_PRODUCTION_DISTRIBUTION.md`** — candidate shippable acquisition/licensing/SBOM/update/rollback routes; route selection still pending;
- **`decisions/ADR-0004-pdf-engine-responsibilities.md`** — remains **Proposed** until final responsibility assignment and explicit owner `N2 PASS`;
- **`../tests/fixtures/pdf/README.md`** — public/synthetic/private fixture rules.

Current evidence has closed core correctness, navigation, Unicode/search, malformed/password behavior, repeated synthetic performance, vector fidelity, coordinate normalization, qpdf preservation/security/transformation, and stress/concurrency. The remaining N2 gates are production-route freeze, explicit scope review, final responsibility assignment, final exact-head CI and owner acceptance.

N2 does not authorize production Reader UI, SQLite/FTS5, scanner, bookmarks editor/local overlay, annotations, migration, installer, OCR or AI document analysis. Those remain later checkpoints.

## Accepted N1 evidence

N1 Windows/toolchain and empty-shell evidence is frozen in:

- **`baselines/N1_WINDOWS_BASELINE.md`** — investigation and accepted baseline summary;
- **`baselines/N1_FINAL_QUALIFICATION.md`** — final owner qualification;
- **`baselines/N1_WINDOWS_FONT_BACKEND_DECISION.md`** — measured Windows GDI shell decision and revisit conditions.

The accepted N1 user-qualified runtime is commit `73f567cf2c557f185371d7f944ebf6d69453105a`.

## Delivery

- **`RELEASE_STRATEGY.md`** — `2.0.0-alpha` → beta → RC → stable rules.
- **`DEVELOPMENT_WORKFLOW.md`** — branches, PRs, commits, ADRs, AI/agent workflow, CI tiers and checkpoint immutability.
- **`PLATFORM_ROADMAP.md`** — Windows → Android → Linux → macOS → iOS/iPadOS.
- **`MIGRATION_FROM_FLUTTER.md`** — side-by-side behavior and legacy profile migration policy.
- **`N0_HANDOFF.md`** — frozen record of the accepted bootstrap review criteria.
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
3. The active checkpoint plan/evidence sheet defines current experiment criteria and evidence.
4. `FEATURE_SCOPE_2_0.md` defines accepted product scope.
5. `PLAN.md` defines principles/north star.
6. `DATA_MODEL_AND_FORMATS.md` defines Atlas-owned persisted/interchange semantics.
7. `TOOLCHAIN.md` defines the accepted Windows alpha toolchain baseline unless an active experiment explicitly records an additional toolchain.
8. Specialized docs define their domain contract.
9. Older prose must be corrected rather than used to justify contradictory behavior.

Never infer that a Planned requirement is already shipped, or that a probe dependency is production-selected.

## Change discipline

Architecture, P0 scope, durable data formats, release gates, canonical toolchain, or dependency strategy must not change silently. Update the relevant specialized document, add/update an ADR when the decision is architectural, revise checkpoint evidence, and note material changes in `CHANGELOG.md`.

Every meaningful N2 engine experiment must record exact engine version/revision, fixture IDs, build/acquisition path, checksums where applicable, measurements, limitations/failures and decision impact in GitHub Markdown.
