# Atlas Reader — Rules for Coding Agents and Contributors

<!-- atlas-status: N4|in-progress -->

Read this before changing anything. AI-written code gets no lower bar than human code.

## 1. Start every session

1. Read [CHECKPOINTS.md](CHECKPOINTS.md) → **Status now**. Work only on the active sub-stage (currently **N4.3**), in order.
2. Read the stage section, the [N4 evidence ledger](docs/baselines/N4_READER_FOUNDATION_EVIDENCE.md), and only the docs relevant to your change:
   - UI → [DESIGN.md](docs/DESIGN.md) and [FOXIT_PARITY.md](docs/FOXIT_PARITY.md) (layout, sizes, shortcuts; never Foxit's files or icons) · threads/engines/layers → [ARCHITECTURE.md](docs/ARCHITECTURE.md)
   - identity, bookmarks, saving, import/export, passwords → [DATA_AND_SAFETY.md](docs/DATA_AND_SAFETY.md)
   - tests and speed → [QUALITY.md](docs/QUALITY.md) · new library or tool → [DEPENDENCIES.md](docs/DEPENDENCIES.md)
3. Check `git status --short --branch` and `git log -5 --oneline`. Never reset, clean or force-push someone's work.
4. Respect the stage's **Excluded** list. Don't start later-stage work because you're "already in that file".

## 2. Architecture (never break)

- Core is portable C++23: no Win32, QML, OS handles or paths, or Qt GUI types in domain contracts.
- QML is presentation only. Identity, reconciliation, PDF security, SQL and save policy never live in QML/JavaScript.
- PDF access goes only through Atlas-owned facades: Qt PDF for reading and rendering, qpdf for structure, security and writes ([ADR-0004](docs/decisions/ADR-0004-pdf-engine-responsibilities.md)). Engine types never cross the boundary.
- Database access goes through repositories; the UI never emits SQL.
- **Never on the UI thread:** directory traversal, PDF parse/render/save, hashing, covers, large SQL, import/export, reconciliation.
- A decision that constrains the future (formats, schema, identity, threading, dependency strategy) needs an ADR in `docs/decisions/`.

## 3. Document safety (never break)

- Never bypass passwords, permissions or DRM. Passwords never reach disk, logs, exports or test snapshots.
- Never silently modify a signed or certified original. Never overwrite a file that changed outside Atlas.
- Never report local work as "Embedded" before a validated write and reopen.
- An unavailable folder is not proof that its books were deleted. Ambiguous identity never moves research to another copy.
- N4 is read-only: source PDF hashes must not change.
- Mutation tests use copied fixtures only. Private PDFs, titles, paths and text never enter Git or CI logs.

## 4. Design and accessibility (every UI change)

- Use `Theme` tokens and shared components; no literal colors or sizes in views. Every visible or accessible string comes from `I18n.t("key")` with entries in **English, Arabic and Urdu** ([ADR-0006](docs/decisions/ADR-0006-interface-languages.md)); never hard-code text. User content must accept and preserve any script.
- Every control is keyboard-operable, has an accessible name, role and state, shows visible focus, works at 200% and in RTL, and doesn't rely on color alone.
- Look at the rendered result in all required states before saying it's done ([DESIGN.md §10](docs/DESIGN.md#10-design-review)). Code alone isn't evidence.

## 5. Evidence

- Every behavior change comes with the smallest test that proves it. Every defect from a review gets a regression test or a numbered manual check.
- Hot-path changes need Release-build measurements: hardware, fixture, percentiles. Never claim "zero lag".
- Use statuses exactly: **Planned · Implemented · Verified · Accepted · Released**. Only the owner's written `N# PASS` makes a stage Accepted.
- Never mark something done from documentation alone, and never upgrade a status because code compiles.

## 6. Dependencies and tools

Don't add a library, service, telemetry, plugin or GitHub app because it's "useful". Show the problem, why Qt/the standard library/existing code can't solve it, the license and the exit plan ([DEPENDENCIES.md](docs/DEPENDENCIES.md)). Development-only tools never become runtime dependencies. Use local CodeGraph (`.codegraph/`, ignored) for navigation when present. Keep credentials in the environment, never in files.

## 7. Before you push

```powershell
pwsh -NoProfile -File tools/docs/check-markdown.ps1
git diff --check
cmake --build --preset windows-release; ctest --preset windows-release
```

Then follow [WORKFLOW.md](docs/WORKFLOW.md): one intent per commit, update docs together with behavior, and confirm the local head equals the remote head after pushing.

## 8. Keep documents in sync

When status, scope or a contract changes, update in the same commit: `README.md`, `PLAN.md` (decision log), `CHECKPOINTS.md`, `CHANGELOG.md`, the evidence ledger, and the affected `docs/` file. All live status markers must match (`<!-- atlas-status: … -->`). Files under `docs/history/` are frozen and are never edited.
