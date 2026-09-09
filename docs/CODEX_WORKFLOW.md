# Codex workflow for Atlas Reader

Atlas uses a deliberately small toolset. Repository facts live in `AGENTS.md`; reusable work methods live in personal Codex Skills; hard secret boundaries live in global hooks.

## Active tools and Skills

- **CodeGraph:** local architecture discovery, source locations, caller/callee paths, and impact analysis. Query it before broad repository searches.
- **GitHub MCP:** optional read-only repository metadata through a user-supplied token. It is development tooling, not an Atlas runtime dependency.
- **Flutter quality CI:** formatting, analysis, tests, and coverage artifacts on pushes and pull requests.
- **Windows beta build CI:** cached release compilation and a complete downloadable runtime bundle on beta tags or manual runs.
- **Existing Codex Skills:** `review`, `review-security`, and `babysit` cover code review, security review, CI failures, and review-comment follow-up without local duplicates.
- **Personal workflow Skills:** `flutter-feature-delivery`, `reader-ui-review`, `pdf-interoperability-audit`, and `beta-release-delivery` activate from their descriptions for feature, UI, PDF, and release work.

The OpenAI PDF Skill is useful for local development-time rendering and round-trip inspection. It is not integrated into Atlas itself and does not add a paid or online runtime dependency.

## Deliberately not added

- Greptile and Repomix duplicate local CodeGraph/source discovery for this repository and add extra indexing or data-handling cost.
- Qodo, Sweep, Sourcery, and similar hosted AI reviewers are unnecessary while built-in review Skills and GitHub CI cover the current workflow.
- Release Drafter is deferred until pull-request labels and release cadence make generated drafts more reliable than the maintained changelog.
- CodeQL Autofix is not used as a substitute for Flutter/Dart analysis; additional security scanning can be reconsidered when it has useful Dart coverage.
- Renovate, Codecov, GitGuardian, and other hosted apps remain optional repository-owner choices because they require account installation or external data processing.

## Secret boundary

The personal hook script was tested with synthetic inputs: ordinary commands
were allowed, environment enumeration and secret-file reads were denied, and
displaying a named environment value requested approval. Configuration currently
lives under `.cursor`; enforcement by the running Codex host has not been
verified. It must not be treated as a complete sandbox or a guarantee against
arbitrary subprocess access. `AGENTS.md` also states the secret-access policy.
Personal hook files are intentionally not committed.

The four personal Skills now appear in Codex's available-Skills catalog, which
confirms discovery. Invoke one explicitly with `$beta-release-delivery` (or its
corresponding name), or use an ordinary matching request. Edit the personal
`SKILL.md` under the Codex skills directory to reuse the workflow elsewhere.
The optional Python validator could not run because PyYAML was unavailable.

After installing or editing MCP configuration, Skills, or hooks, restart Codex so every component reloads.
