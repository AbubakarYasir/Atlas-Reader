# Atlas Reader — Development, Git and Release Workflow

The goal is fast progress without losing discipline. Atlas is built stage by stage ([CHECKPOINTS.md](../CHECKPOINTS.md)), never on one long branch that becomes impossible to review.

## 1. Branches

| Branch | Purpose |
|---|---|
| `main` | Default branch. Accepted stage state only. Permanent name. |
| `legacy/flutter` | Stopped Flutter app, kept as a reference. Permanent name. Never revived. |
| `native-v2-n<#>-<name>` | The **one** active stage branch, created from the exact accepted `main` commit. Today: `native-v2-n4-reader-foundation`. |
| `<area>/<short-name>` | Short-lived task branch from the stage branch, merged back into it (for example `docs/focused-2-0-plan`, `reader/viewport-cache`). |
| `spike/<name>` | Experiments, never merged silently into production architecture. |

No detached HEAD and no hidden clones or worktrees. `git status` must explain the full state. Stage branches are deleted after their accepted merge, once ancestry is verified. `archive/*` tags may preserve unmerged history before deleting an obsolete branch; they are never releases.

## 2. Starting work

```powershell
git status --short --branch
git branch --show-current
git log -5 --oneline --decorate
git fetch --prune   # read-only comparison
```

Never reset, clean, overwrite or force-push other work to make the tree look tidy.

## 3. Commits

Commits are authored by the owner and **signed** (SSH signing key registered on the owner's GitHub account), so every commit shows **Verified**. Commits carry no AI co-author or session lines, whoever drafted them.

One reviewable intent per commit, with a prefix: `build:` `core:` `pdf:` `index:` `reader:` `bookmarks:` `ink:` `a11y:` `i18n:` `design:` `platform:` `test:` `docs:` `fix:`. Don't mix dependency upgrades, rewrites, redesigns and features in one commit.

Before committing:

1. Read the whole diff. Check for secrets, private PDF names, paths or text, build output and unrelated changes.
2. Run the smallest relevant checks plus the stage gates (see [AGENTS.md §7](../AGENTS.md#7-before-you-push)).
3. Update docs together with behavior: status, evidence, changelog, and the affected contract.
4. Record limitations with an owning stage and a blocking test.

Documentation-only changes don't bump the product version, but they still need the docs check and a pushed branch.

## 4. Push, pull requests and merge

1. Push the branch. It is not "on GitHub" until `git rev-parse HEAD` equals `git rev-parse origin/<branch>`.
2. The stage PR stays **Draft** until the stage's owner review passes. It is currently PR #6 for N4.
3. CI must pass on the **exact** head; an older green run doesn't count.
4. After **`N# PASS`**: update the ADR and status files, merge stage → `main`, verify ancestry, tag the release, delete the stage branch, then open the next stage branch from the merge commit. The owner may direct an earlier merge when a stage's build is complete (N4, 2026-11-02); the review then runs on `main`, and the release tag still waits for `N# PASS`.
5. Never force-push a published branch without explicit owner permission; prefer corrective commits.

**PR description:** stage and requirement · what changed · boundaries touched · dependencies · tests run (with run IDs) · performance evidence for hot paths · schema/format changes · accessibility/RTL impact and screenshots · known limitations with their owning stage.

**CI:** Windows CI (Debug + Release, docs governance, script syntax, review package on the N4 branch) runs on every push. The seven inherited N2 PDF regression workflows run on `main` and PRs. N4 will path-filter them and add a nightly schedule ([CHECKPOINTS.md](../CHECKPOINTS.md#engineering-speed-tasks-in-n4)).

## 5. Versions and releases

Semantic Versioning, all on the `2.0.0` line until stable:

- `2.0.0-alpha.N`: engineering qualification (N0–N2, done).
- `2.0.0-beta.N`: each usable stage (N3 → beta.1 … N9 → beta.7).
- `2.0.0-rc.N`: N10. `2.0.0`: N11, byte-equivalent to the accepted RC.
- Corrective builds: `2.0.0-beta.3+1`, or the next beta when behavior changes. **Never retag different bytes.**
- After 2.0: feature releases `2.x` and platform releases, ordered in [AFTER_2_0.md](AFTER_2_0.md).

Every distributable records: commit, source version, compiler and Windows SDK, Qt, PDF engine and dependency versions with the vcpkg baseline, build configuration, test totals, a relevant benchmark summary, known limitations, SHA-256, third-party notices and SBOM, and owner status.

**Publisher:** DerekSol ([dereksol.com](https://dereksol.com)). The Windows executable carries DerekSol as company, copyright and description (`assets/windows/atlas_reader.rc.in`), and About shows DerekSol's contact.

**Code signing:** Windows shows "Unknown publisher" for any program that is not signed with an Authenticode code-signing certificate, whatever its file details say. Removing it needs a certificate issued to DerekSol (an OV/EV certificate from a certificate authority, or Microsoft Trusted Signing / Azure Artifact Signing, which verifies the business). Once DerekSol has one, release builds are signed with `signtool sign /fd SHA256 /tr <timestamp URL> /td SHA256` in CI with the certificate kept in GitHub secrets, never in the repository. Planned for N11 (installer); earlier if the certificate is ready.

**Status words** are used exactly: Planned · Implemented · Verified · Accepted · Released. The README advertises only Released features or clearly labeled beta behavior.

## 6. ADRs

Write an ADR for changes to: the UI framework or language, the PDF responsibility split, the dependency manager, the database or search model, persisted or backup formats, the identity algorithm, the threading model, installer/update technology, licensing/distribution, or major storage abstractions. States: Proposed → Accepted → Superseded/Rejected. Never renumber or delete one. See [decisions/README.md](decisions/README.md).

## 7. Dependencies

Never auto-merge dependency bumps. Each update records old and new versions, security reason, license change, API/ABI impact, binary size, benchmark delta, fixture results and rollback ([DEPENDENCIES.md](DEPENDENCIES.md)).

## 8. AI agents

Agents accelerate work; they are not authorities. They follow [AGENTS.md](../AGENTS.md), use CodeGraph for navigation when present, never add dependencies on their own initiative, never touch secrets, and face the same acceptance bar: compiles, tests pass, follows the architecture, has evidence and docs. Speculative refactors are not accepted just because an agent proposes them.

## 9. Backlog

Every idea goes into exactly one bucket: active stage · next stage · Windows 2.0 (with a stage) · [after 2.0](AFTER_2_0.md) · rejected. Nothing is smuggled into the active stage.

## 10. Handoff record

Every pushed handoff states: branch and PR · local and remote SHA (equal) · worktree state · what changed · tests and CI on that SHA · artifact and checksum if any · limitations with owners · whether owner PASS, merge or tag are still pending.
