# Atlas Reader Native — Risk Register

Atlas Reader 2.0 is a native rewrite of a data-sensitive document application. The main project risk is not lack of ideas; it is losing focus, portability, safety, or performance while trying to match mature products.

This register is reviewed at every checkpoint. Risks may be added, retired, or reprioritized with evidence.

## Risk scale

- **Critical** — can invalidate the release or architecture.
- **High** — likely to cause major delay, data/safety problems, or a poor core workflow.
- **Medium** — significant but containable within a checkpoint.
- **Low** — monitor; not currently release-threatening.

## Current risks

| Risk | Level | Why it matters | Mitigation / gate |
|---|---|---|---|
| PDF engine limitations discovered late | Critical | Rendering may look fine while text, destinations, permissions, save preservation, or Arabic edge cases fail | N2 engine bake-off before final reader; Atlas-owned facade; real fixture corpus; ability to split rendering and mutation engines |
| Silent PDF/data corruption | Critical | Atlas stores research in/around user documents | temp→validate→replace→reopen save pipeline; preservation tests; failure injection; local fallback; no false success |
| External document overwrite | Critical | Acrobat/Foxit/cloud sync may change a PDF while Atlas has it open | source revision fingerprint before every mutation; conflict/reconciliation rather than blind save |
| Local-overlay identity attaches to wrong copy | Critical | Could expose or destroy another book's research state | guarded identity model; no filename-only reassociation; ambiguity remains unresolved until user/strong evidence |
| Qt/open-source licensing mistake | Critical | Could block distribution after major implementation work | module/dependency license inventory at admission time; dynamic linking/relinkability policy; SBOM/notices; N10 legal/distribution review |
| Rewrite scope explosion | High | Competing by total Acrobat/Foxit feature count can prevent shipping | binding P0/P1/P2/deferred feature scope; Index→Reader→Bookmarks order; checkpoint stop gates |
| C++ memory-safety defects | High | Complex native parsing/rendering integration increases crash/security risk | RAII/value types; standard library; sanitizers; static analysis; narrow third-party boundaries; fuzz/adversarial fixtures where valuable |
| UI thread stalls | High | The rewrite loses its main reason to exist if reading/indexing blocks interaction | explicit thread contract; bounded queues; profiling; performance budgets from N1 onward; no sync parse/render/save/scan/large SQL on UI thread |
| Long-document memory blow-up | High | Eager page rendering can make large scholarly PDFs unusable | virtualized page model; bounded CPU/GPU cache; benchmark 2,000+ page fixtures; memory budgets |
| Qt 6.11 public CI/toolchain inconsistency | High until N1 | Local and CI builds could diverge | N1 selects one reproducible canonical toolchain; current bootstrap CI uses compatible public 6.10.3 only as temporary verification pin |
| Poor PDF interoperability | High | Atlas data could become portable only inside Atlas | standard PDF structures; Acrobat/Foxit/Okular/Xournal++/other-reader round-trip fixtures where applicable; never claim compatibility without evidence |
| Signed/certified document invalidation | High | Mutation can invalidate signatures or violate expected integrity | signed/certified capability state; local-only default; explicit working-copy flow; never silently alter signed original |
| Weak bookmark model becomes schema debt | High | Bookmarks are a signature feature and need deep hierarchy, overrides, conflicts, portability | domain model and storage states defined before schema; SQLite is implementation detail; versioned interchange format |
| Index watcher event storms / duplicated work | Medium/High | Large libraries can consume CPU/storage and hurt reader responsiveness | event coalescing/debounce; bounded background priority; progressive scan; database writer queue; cancellation |
| Windows-first code leaks into core | High | Android/macOS/iOS later become rewrites | platform ports; standard C++ domain; no Win32 path/handle types in core; ADR required for exceptions |
| Android/iOS storage models expose flawed path assumptions | High | Mobile file access is URI/document-provider/security-scope based, not desktop paths | logical document references through storage adapters; platform roadmap reviews abstractions before Windows 2.0 stable |
| Accessibility added too late | High | Custom canvas/tree controls can be difficult to retrofit | semantics/keyboard/RTL from first user-facing checkpoint; N9 is qualification, not first implementation |
| Arabic/RTL edge cases hidden by English development | High | A key target workflow would fail despite general correctness | Arabic/mixed-script fixtures in every relevant checkpoint; bidi isolation; native RTL UI testing |
| Migration from Flutter loses local research | High | Existing Atlas users cannot trust V2 | legacy DB read-only importer; preview; idempotency; backup; unresolved identity surfaced; never modify legacy profile |
| Backup format becomes tied to internal SQLite | Medium/High | Future schema changes make restore brittle | versioned Atlas-owned logical backup records; consistent snapshot; independent validation; migration fixtures |
| Dependency abandonment / license change | Medium | External code can become future lock-in | minimal dependency policy; Atlas-owned interfaces; pinned baselines; update review; replacement plan for architecture-significant dependencies |
| Over-engineering before user value | Medium | Native rewrite can become architecture work indefinitely | N0/N1/N2 are explicitly limited; first useful beta is N3; no speculative framework layers without active checkpoint need |
| Visual polish mistaken for product readiness | Medium | Attractive UI can hide corruption/performance/accessibility defects | acceptance hierarchy in `SUCCESS_METRICS.md`; evidence gates precede stable |
| No representative low-end hardware testing | Medium | Developer RTX/high-end hardware can hide inefficient code | N10 hardware matrix includes baseline/representative slower storage/GPU/display; budgets documented per machine |
| Diagnostics leak sensitive document information | High | Local scholarly/private documents may expose text/paths/passwords | privacy-safe logs; no document contents/passwords; diagnostic-bundle preview/redaction; no telemetry in 2.0 |
| Installer/uninstaller damages user data | Critical at N11 | Release packaging can undo all application safety work | app data/books separate; uninstall never deletes user PDFs/research backups; install/upgrade/uninstall regression matrix |

## Checkpoint-specific risk review

Before accepting each checkpoint, add a short section to its evidence report:

- new risks discovered;
- changed likelihood/impact;
- mitigations implemented;
- risks consciously accepted;
- risks that must block the next checkpoint.

## Architecture escape hatch

A previous decision may be changed when evidence proves it wrong. The process is:

1. reproduce the problem;
2. show why the existing boundary/technology cannot meet the accepted requirement reasonably;
3. compare alternatives;
4. measure migration cost and compatibility impact;
5. write/update an ADR;
6. preserve user data formats or provide a tested migration;
7. re-run affected checkpoint gates.

We do not protect a bad decision merely because it was documented earlier.

## Product risk principle

Atlas does not need to become the largest PDF suite. The safest competitive position is to become exceptionally dependable in the workflows it owns:

**find the book → read it smoothly → navigate/research it deeply → preserve that research safely.**

Any feature that threatens those four steps carries a higher burden of proof.