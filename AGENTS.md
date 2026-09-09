<!-- CODEGRAPH_START -->
## CodeGraph

In repositories indexed by CodeGraph (a `.codegraph/` directory exists at the repo root), reach for it BEFORE grep/find or reading files when you need to understand or locate code:

- **MCP tool** (when available): `codegraph_explore` answers most code questions in one call — the relevant symbols' verbatim source plus the call paths between them, including dynamic-dispatch hops grep can't follow. Name a file or symbol in the query to read its current line-numbered source. If it's listed but deferred, load it by name via tool search.
- **Shell** (always works): `codegraph explore "<symbol names or question>"` prints the same output.

If there is no `.codegraph/` directory, skip CodeGraph entirely — indexing is the user's decision.
<!-- CODEGRAPH_END -->

## Atlas Reader product rules

- Keep the application runtime offline/local-first, free, and open source.
- Treat PDFs as the portable source of truth for outlines and standard annotations; local SQLite data is an index/projection unless the feature is inherently app-local.
- Prioritize Windows quality while keeping platform-specific filesystem and launch behavior behind adapters so Android and other platforms remain viable.
- Preserve Arabic/RTL, keyboard access, screen-reader semantics, focus visibility, and text scaling in user-facing changes.
- Prefer progressive, bounded background work for library/PDF processing and keep the Flutter UI isolate responsive.
- Keep `README.md`, `PLAN.md`, `CHANGELOG.md`, and `INFO.md` consistent with verified implementation. Use beta SemVer and do not mark unfinished work complete.
- Never inspect or enumerate environment values or secret files. Narrowly consume a specifically required secret without displaying it only when the task clearly requires it.
