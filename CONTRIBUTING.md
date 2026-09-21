# Contributing

Atlas Reader Native is checkpoint-driven. Before contributing, read `AGENTS.md`, `PLAN.md`, and the active section of `CHECKPOINTS.md`.

## Principles

- Keep changes within the active checkpoint.
- Prefer small, testable domain changes over UI-first feature patches.
- Add tests with new behavior.
- Measure performance-sensitive changes.
- Preserve Arabic/RTL/accessibility for user-facing work.
- Keep platform-specific implementation behind ports.
- Update documentation when behavior/contracts change.

## Code style

- C++23.
- `clang-format` using repository `.clang-format`.
- Prefer RAII, value semantics, `std::unique_ptr` for ownership, `std::span`/views where appropriate, and explicit lifetime boundaries.
- Avoid shared ownership unless the domain genuinely requires it.
- Avoid exceptions crossing subsystem/plugin boundaries without an explicit error contract.
- Core APIs should use standard types and stable value objects rather than GUI framework objects.

## Pull request evidence

A PR should state:

- checkpoint and requirement addressed;
- tests added/run;
- performance impact when relevant;
- platform implications;
- PDF/data-safety implications;
- accessibility/i18n implications;
- documentation changes.

Do not merge feature work that advances the next checkpoint before the current owner stop gate is accepted.