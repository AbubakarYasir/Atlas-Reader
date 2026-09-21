# Atlas Reader Native — Release Strategy to 2.0

## Purpose

This document defines how the native successor advances from engineering bootstrap to a stable **2.0.0** Windows release without confusing experiments with user-ready builds.

## Versioning model

Atlas Reader Native follows Semantic Versioning. The native rewrite is a new major product generation, so all native preview builds belong to the **2.0.0** line.

### Internal alpha phase

`2.0.0-alpha.N` is for architecture/toolchain/engine qualification before the product is useful enough for ordinary reading work.

- N0 — `2.0.0-alpha.0`: repository/bootstrap only; not a user release.
- N1 — `2.0.0-alpha.1`: reproducible Windows shell and performance baseline.
- N2 — `2.0.0-alpha.2`: PDF-engine qualification/prototypes; not the final reader.

Alpha artifacts may be attached to CI for engineering inspection but are not promoted as normal beta downloads.

### Beta phase

A beta must be usable for a real Atlas workflow. Betas begin when Library/Index exists.

| Checkpoint | Planned version | User value |
|---|---|---|
| N3 | `2.0.0-beta.1` | Native library/index/search foundation |
| N4 | `2.0.0-beta.2` | Native PDF reader foundation |
| N5 | `2.0.0-beta.3` | Complete resilient bookmarks/outlines |
| N6 | `2.0.0-beta.4` | Ink and standard annotations |
| N7 | `2.0.0-beta.5` | Printing, covers, metadata |
| N8 | `2.0.0-beta.6` | Flutter migration + first-class backup/restore |
| N9 | `2.0.0-beta.7` | Arabic/RTL/accessibility qualification |

A checkpoint may require more than one beta build. Corrective builds use build metadata or an incremented beta number; a failed build is never silently reused.

Examples:

- `2.0.0-beta.3+1`
- `2.0.0-beta.3+2`
- or, when behavior changes materially, `2.0.0-beta.4`

## Release-candidate phase

N10 performance/hardware qualification produces the first **release candidate** only after all planned Windows 2.0 features are feature-complete.

- `2.0.0-rc.1`
- `2.0.0-rc.2` only if fixes change the artifact

The release candidate must use the same compiler family, dependency pins, packaging process, migration path, and data formats intended for stable release.

## Stable Windows 2.0

N11 promotes an accepted release candidate to:

`2.0.0`

No code, dependency, installer, translation, schema, or packaging change may occur between the final accepted RC artifact and stable without producing another RC and repeating affected qualification.

## What happens after Windows 2.0

Android, Linux, macOS, and iOS/iPadOS are platform-adaptation programs, not reasons to hold Windows 2.0 indefinitely.

Recommended platform preview lines:

- Android: `2.1.0-alpha/beta` → `2.1.0`
- Linux: `2.2.0-alpha/beta` → `2.2.0`
- macOS: `2.3.0-alpha/beta` → `2.3.0`
- iOS/iPadOS: `2.4.0-alpha/beta` → `2.4.0`

The exact minor numbers may change, but each platform is accepted only when its adapter/UI/hardware qualification passes without forking core product rules.

## Release evidence

Every distributable beta/RC/stable build records:

- commit SHA;
- source version;
- compiler and Windows SDK;
- Qt version;
- PDF engine versions/configuration;
- qpdf/SQLite and other shipped dependency versions;
- dependency lock/baseline identifier;
- build configuration;
- test totals;
- benchmark summary relevant to that checkpoint;
- known limitations;
- SHA-256 checksum;
- third-party notices/SBOM;
- owner acceptance status.

## Branch/tag policy

- `main` in the eventual native repository contains only accepted checkpoint state.
- Feature work occurs on short-lived branches.
- Tags are immutable.
- Never retag a different artifact with the same version.
- Experimental PDF-engine spikes stay on branches until a decision record accepts them.

## Status vocabulary

Documentation must use these words precisely:

- **Planned** — requirement documented, no implementation claim.
- **Implemented** — code exists.
- **Verified** — automated/manual evidence recorded.
- **Accepted** — owner has explicitly passed the checkpoint.
- **Released** — an accepted artifact is published under a version/tag.

The README may advertise only what is Released or clearly label beta/experimental behavior.