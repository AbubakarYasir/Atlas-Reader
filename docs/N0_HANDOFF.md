# N0 Handoff — Native Repository Bootstrap

**Checkpoint:** N0 — Native repository bootstrap  
**Release label:** `2.0.0-alpha.0` engineering bootstrap  
**State:** Verified / Ready for owner review  
**Next checkpoint:** N1 — Windows toolchain + empty-shell baseline  

N1 must not begin until the owner records **PASS** for N0.

## What N0 intentionally delivers

- clean C++23/CMake/Qt Quick native source tree;
- minimal QML shell only;
- portable core contracts and platform/PDF/index boundaries;
- Windows CI that configures, builds, and runs the smoke test;
- complete Windows 2.0 product/engineering program documentation;
- release sequence from alpha → beta → RC → 2.0 stable;
- explicit feature scope and non-goals;
- PDF safety/local-fallback/reconciliation contract;
- data/interchange/backup identity rules;
- open-source dependency/tool/plugin catalog and adoption gates;
- quality/performance/security/accessibility/Arabic requirements;
- risk register and competitive baseline;
- agent/AI contribution rules;
- platform roadmap after Windows.

## What N0 intentionally does not deliver

- production PDF engine;
- SQLite schema/index implementation;
- folder scanner/watcher;
- production Library UI;
- PDF reader implementation;
- bookmark editor;
- annotation engine;
- migration implementation;
- installer.

Those belong to later checkpoints and should not be pulled into N0.

## Owner review path

Read in this order:

1. `README.md`
2. `PLAN.md`
3. `CHECKPOINTS.md`
4. `docs/FEATURE_SCOPE_2_0.md`
5. `docs/SUCCESS_METRICS.md`
6. `docs/ARCHITECTURE.md`
7. `docs/CORE_WORKFLOWS.md`
8. `docs/DATA_MODEL_AND_FORMATS.md`
9. `docs/TECH_STACK.md`
10. `docs/DEPENDENCIES_AND_TOOLS.md`
11. `docs/UPSTREAM_CATALOG.md`
12. `docs/PERFORMANCE.md`
13. `docs/QUALITY_AND_TESTING.md`
14. `docs/UX_ACCESSIBILITY_AND_DESIGN.md`
15. `docs/SECURITY_MODEL.md`
16. `docs/COMPETITIVE_BASELINE.md`
17. `docs/RISK_REGISTER.md`
18. `docs/RELEASE_STRATEGY.md`
19. `docs/DEVELOPMENT_WORKFLOW.md`
20. `docs/PLATFORM_ROADMAP.md`
21. `docs/MIGRATION_FROM_FLUTTER.md`
22. `docs/LICENSING.md`

`docs/README.md` is the permanent documentation index.

## Questions N0 is asking the owner to accept

- Is **Index → Reader → Bookmarks** the correct product priority?
- Is Windows-first, then Android → Linux → macOS → iOS/iPadOS, the accepted platform order?
- Is C++23 + Qt Quick/QML + CMake the accepted native foundation?
- Is the PDF-engine abstraction/bake-off approach acceptable rather than hard-wiring one engine now?
- Is SQLite/FTS5 the accepted local index/search direction?
- Is vcpkg manifest mode acceptable for qualified non-Qt C/C++ dependencies?
- Is **portable when possible, local when necessary, never lost silently** the accepted research-data rule?
- Is the protected/read-only/signed/conflict local-overlay behavior acceptable?
- Is the alpha/beta/RC sequence to Windows `2.0.0` acceptable?
- Is the P0/P1/P2/deferred feature scope acceptable?
- Is the requirement to preserve Arabic/RTL/accessibility from early user-facing checkpoints acceptable?
- Are the documented safety/performance/quality gates strict enough?
- Is it acceptable that Atlas competes on the workflows it owns instead of matching Acrobat/Foxit feature-for-feature?

## Automated evidence

The bootstrap Windows workflow has already demonstrated the complete configure → Release build → CTest path successfully. Subsequent documentation-only commits must continue to leave branch-head CI green before N0 is accepted.

N1 owns exact canonical product toolchain qualification, including resolving the preferred Qt 6.11.x patch versus the temporary public-CI compatibility pin.

## Acceptance record

Leave this checkpoint unchanged until explicit owner review.

- **PASS** — mark N0 Accepted; open N1 only.
- **CHANGES REQUIRED** — keep N0 active and list the required corrections.

Do not infer PASS from silence, a successful CI run, or approval of an individual technology choice.