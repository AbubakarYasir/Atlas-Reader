# Contributing

Atlas is built stage by stage. Before contributing, read [AGENTS.md](AGENTS.md), the **Status now** section of [CHECKPOINTS.md](CHECKPOINTS.md), and [docs/WORKFLOW.md](docs/WORKFLOW.md).

## Principles

- Stay within the active sub-stage and respect its exclusions.
- Prefer small, testable domain changes over UI-first patches; add tests with every behavior change.
- Measure performance-sensitive changes in Release builds.
- Every UI change follows [docs/DESIGN.md](docs/DESIGN.md): tokens, translations, keyboard, Narrator, 200%, Arabic/RTL, rendered review.
- Keep platform code behind ports and PDF engines behind Atlas facades.
- Update documentation in the same change when behavior or contracts change.

## Code style

C++23 · `clang-format` with the repository `.clang-format` · RAII and value semantics · `std::unique_ptr` for ownership, `std::span`/views where they fit · shared ownership only when the domain needs it · no exceptions crossing subsystem boundaries without an explicit error contract · core APIs use standard types, not GUI objects.

## Pull requests

State the stage and requirement, the tests run (with CI run IDs), performance impact, platform, PDF/data-safety and accessibility/i18n implications, and the docs changed. Before pushing, run `tools/docs/check-markdown.ps1` and the relevant build and tests, read the full diff, and confirm the pushed head equals your local head. Owner PASS, merge and release are separate steps.

Security issues: please follow [SECURITY.md](SECURITY.md), not public issues.
