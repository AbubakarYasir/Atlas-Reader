<!-- CODEGRAPH_START -->
## CodeGraph

In repositories indexed by CodeGraph (a `.codegraph/` directory exists at the repo root), reach for it BEFORE grep/find or reading files when you need to understand or locate code:

- **MCP tool** (when available): `codegraph_explore` answers most code questions in one call — the relevant symbols' verbatim source plus the call paths between them, including dynamic-dispatch hops grep can't follow. Name a file or symbol in the query to read its current line-numbered source. If it's listed but deferred, load it by name via tool search.
- **Shell** (always works): `codegraph explore "<symbol names or question>"` prints the same output.

If there is no `.codegraph/` directory, skip CodeGraph entirely — indexing is the user's decision.
<!-- CODEGRAPH_END -->

## Atlas Reader product rules

- Keep the application runtime offline/local-first, free, and open source.
- Treat PDFs as the preferred portable source of truth for outlines and standard annotations when they can be modified safely and legitimately; use an explicit local Atlas overlay when the source is read-only, restricted, signed/certified, externally locked, conflicted, or otherwise unsafe to mutate.
- Before changing Library/Index, Reader, Bookmarks/Outline, save, sync, or document-capability behavior, read `docs/CORE_WORKFLOWS.md`. Its capability, fallback, conflict, portability, and recovery contracts are planned product requirements and must not be silently weakened.
- Never bypass PDF passwords or permission restrictions, silently invalidate signed/certified documents, or overwrite a source revision that changed outside Atlas. Preserve user work locally and surface a recovery path instead.
- Prioritize Windows quality while keeping platform-specific filesystem and launch behavior behind adapters so Android and other platforms remain viable.
- Preserve Arabic/RTL, keyboard access, screen-reader semantics, focus visibility, and text scaling in user-facing changes.
- Prefer progressive, bounded background work for library/PDF processing and keep the Flutter UI isolate responsive.
- Keep `README.md`, `PLAN.md`, `CHANGELOG.md`, and `INFO.md` consistent with verified implementation. Use beta SemVer and do not mark unfinished work complete.
- Never inspect or enumerate environment values or secret files. Narrowly consume a specifically required secret without displaying it only when the task clearly requires it.
