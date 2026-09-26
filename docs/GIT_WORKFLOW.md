# Atlas Reader Native — Git and Documentation Workflow

This is the binding Git-tree policy. Its purpose is to keep one understandable
line of development, preserve accepted checkpoints, and ensure the online
repository matches the reviewed local work.

## Branch model

- `native-v2-bootstrap` contains accepted native checkpoint state. `main` is the frozen Flutter reference and public entry point; Flutter development has stopped.
- One checkpoint branch is active at a time, named
  `native-v2-n<number>-short-name`.
- The checkpoint branch starts from the exact accepted predecessor commit.
- Small task branches may start from the active checkpoint branch and merge
  back into it. They must not contain later-checkpoint work.
- Spikes remain separate until their decision and evidence are accepted.
- Do not work in detached HEAD state. Any additional worktree must have a named
  branch and must be listed in the handoff.
- Never use a second clone or worktree to hide uncommitted or contradictory
  work. `git status` must explain the complete state being delivered.

N2 uses `native-v2-n2-pdf-engine-qualification`. N3 must not be created or
started until the owner records `N2 PASS` and N2 is integrated according to the
accepted PR decision.

## Start-of-work check

Before editing:

```powershell
git status --short --branch
git branch --show-current
git remote -v
git log -5 --oneline --decorate
```

Fetch/prune is read-only and may be used to compare local and remote state. Do
not reset, clean, overwrite, or force-push user work to make the tree look tidy.

## Commit rules

One commit should express one reviewable intent. Use the prefixes documented in
`DEVELOPMENT_WORKFLOW.md`, such as `docs:`, `test:`, `reader:` or `fix:`.

Before committing:

1. inspect `git status` and the complete diff;
2. confirm no secrets, private PDF names/paths/text, generated build output or
   unrelated user changes are included;
3. run the smallest relevant verification plus required checkpoint gates;
4. update behavior, plan, checkpoint, evidence and changelog documentation in
   the same change when they would otherwise disagree; and
5. record limitations using the owner/checkpoint/test format in
   `CHECKPOINT_QA_MATRIX.md`.

Documentation-only corrections do not bump the product version. They still
require link/status validation, a meaningful commit and a pushed branch.

## Push and pull-request lifecycle

1. Push the named checkpoint branch to `origin`; do not claim “on GitHub” until
   the remote branch resolves to the local commit.
2. Keep the checkpoint pull request Draft while implementation/evidence is
   incomplete or owner testing is pending.
3. CI must run on the exact proposed head. A successful older commit does not
   qualify a newer documentation or code head.
4. The owner records `N# PASS` before the checkpoint becomes Accepted.
5. Update the ADR/status ledger and merge only after the owner gate and required
   checks pass. Do not begin the next checkpoint merely because a PR is open or
   CI is green.
6. Never force-push a published checkpoint branch unless the owner explicitly
   authorizes history replacement. Prefer additive corrective commits.

After pushing, verify:

```powershell
git status --short --branch
git rev-parse HEAD
git rev-parse origin/$(git branch --show-current)
```

The two commit IDs must match for a fully synchronized handoff.

## Merge and tag rules

- Merge direction is task branch → active checkpoint branch → `native-v2-bootstrap` after
  acceptance.
- Preserve meaningful checkpoint history; do not mix unrelated future work into
  the acceptance merge.
- Tags are immutable and are created only for an Accepted artifact under
  `RELEASE_STRATEGY.md`.
- Never reuse a version/tag for different bytes.
- Stable promotion requires the accepted RC payload to remain equivalent as
  defined by N11.

## Documentation authority and synchronization

`CHECKPOINTS.md` owns current checkpoint/status. `PLAN.md` owns the north star.
`FEATURE_SCOPE_2_0.md` owns product scope. `CHECKPOINT_QA_MATRIX.md` owns stage
evidence and limitation transfer. The active evidence sheet and ADR own measured
decisions. Historical baseline files remain historical and must carry a clear
superseded note when a later decision changes their provisional conclusion.

Any checkpoint/status/architecture change must review and, when applicable,
synchronize:

- `README.md`;
- `PLAN.md`;
- `CHECKPOINTS.md`;
- `INFO.md`;
- `CHANGELOG.md`;
- `AGENTS.md`;
- `docs/README.md`;
- the active checkpoint plan/evidence;
- affected ADR, dependency, build, release, risk, quality and workflow docs.

Run `tools/docs/check-markdown.ps1` before push. CI repeats this check to catch
broken relative links, divergent live-status markers and missing QA/owner/stop
gates for N3–N11.

## Handoff record

Every pushed handoff reports:

- branch and PR;
- local/remote commit SHA;
- clean or explained worktree state;
- files/behavior changed;
- tests and CI status on that exact SHA;
- artifact/checksum when applicable;
- known limitations with named checkpoint/test owner; and
- whether owner PASS, merge or tag is still pending.

This record is required even when the change is documentation-only.
