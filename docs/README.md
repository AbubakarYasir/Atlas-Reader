# Atlas Reader — Documentation

<!-- atlas-status: N5|in-progress -->

Each topic has exactly one home. If two documents seem to disagree, fix the wrong one; don't follow it.

## Start here

| If you want to… | Read |
|---|---|
| Understand the product and 2.0 scope | [PLAN.md](../PLAN.md) |
| See status and what's next | [CHECKPOINTS.md](../CHECKPOINTS.md) |
| Contribute code (people or agents) | [AGENTS.md](../AGENTS.md) → [WORKFLOW.md](WORKFLOW.md) → [BUILDING.md](BUILDING.md) |
| Review the last accepted stage | [N4_REVIEW.md](N4_REVIEW.md) (N5's guide arrives with its review build) |

## Reference

| Document | Owns |
|---|---|
| [DESIGN.md](DESIGN.md) | Design system, tokens, components, keyboard, accessibility, Arabic/RTL, design review |
| [FOXIT_PARITY.md](FOXIT_PARITY.md) | Measured Foxit workspace layout, skins, shortcuts, and every Foxit command mapped to an Atlas stage |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Technology choices, layers, threads, reader viewport, PDF facade, platform ports |
| [DATA_AND_SAFETY.md](DATA_AND_SAFETY.md) | Identity, bookmark states, safe save, import/export, backup/migration, security model |
| [QUALITY.md](QUALITY.md) | Test layers, fixtures, preservation, failure injection, speed budgets, evidence record |
| [WORKFLOW.md](WORKFLOW.md) | Branches, commits, PRs, CI, versions, releases, ADRs |
| [BUILDING.md](BUILDING.md) | Toolchain, environment, build, test, review package |
| [DEPENDENCIES.md](DEPENDENCIES.md) | What ships, admission rule, candidates, licensing, dev tools |
| [AFTER_2_0.md](AFTER_2_0.md) | Formats, editing tools, extras and platforms after 2.0 |

## Records

- [decisions/](decisions/README.md): architecture decision records (ADR-0001 to ADR-0008).
- [baselines/](baselines/): measured evidence per stage. The active ledger is [N5_BOOKMARKS_SAFETY_EVIDENCE.md](baselines/N5_BOOKMARKS_SAFETY_EVIDENCE.md); N4 is closed ([acceptance](baselines/N4_ACCEPTANCE.md)).
- [tests/fixtures/pdf](../tests/fixtures/pdf/README.md): PDF fixture provenance, privacy and checksum contract.
- [history/](history/README.md): frozen earlier plans, unchanged and not maintained.
- [CHANGELOG.md](../CHANGELOG.md) · [SECURITY.md](../SECURITY.md) · [CONTRIBUTING.md](../CONTRIBUTING.md).

## Authority when in doubt

1. Running code and tests decide what is actually Implemented.
2. [CHECKPOINTS.md](../CHECKPOINTS.md) decides order and status.
3. [PLAN.md](../PLAN.md) decides scope, priorities and principles.
4. Reference docs decide their own domain; ADRs record binding decisions.
5. `history/` is context only and never overrides anything.

Scope, release gates, durable formats, toolchain and dependency strategy never change silently: update the owning doc, add a row to the [decision log](../PLAN.md#9-decision-log) or an ADR, and add a [CHANGELOG](../CHANGELOG.md) entry.
