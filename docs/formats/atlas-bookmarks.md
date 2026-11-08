# Atlas bookmarks file (`.atlas-bookmarks.json`), schema 1.0.0

Status: **Implemented** (N5.5). Machine-readable schema: [atlas-bookmarks.schema.json](atlas-bookmarks.schema.json) (JSON Schema 2020-12). Fixtures: [`tests/fixtures/bookmarks/`](../../tests/fixtures/bookmarks/), checked by `atlas_bookmark_exchange`.

The one **lossless** bookmark export of Atlas Reader: hierarchy, order, exact titles, notes, tags and destinations come back exactly as they were. Markdown and CSV exports exist for reading and spreadsheets; they are never called lossless and are not imported.

## File

UTF-8 JSON, one object:

| Member | Required | Meaning |
|---|---|---|
| `format` | yes | Always `"atlas-bookmarks"`. Anything else is refused. |
| `schemaVersion` | yes | `MAJOR.MINOR.PATCH`. This document describes `1.0.0`. |
| `exporter` | no | `{name, version, exportedUtc}`: informational only. |
| `document` | no | `{title, fileName, pageCount}`: hints about the book the bookmarks came from. **Never** used to pick or bind a file; Atlas only warns when `fileName` differs from the open book. |
| `scope` | no | `all`, `subtree` or `localOnly`: what the export held. Informational. |
| `bookmarks` | yes | Array of bookmarks, in reading order. |

A bookmark:

| Member | Required | Meaning |
|---|---|---|
| `title` | yes | Exact text (spaces, duplicates, any script). At most 16 KiB of UTF-8. |
| `location` | no | Atlas location envelope ([ADR-0008](../decisions/ADR-0008-bookmark-overlay-and-location-envelope.md)), e.g. `atlas-location/1;scheme=pdf;page=3;kind=xyz;left=0.1;top=0.25`. Page index from 0; coordinates are fractions of the page. Missing = a bookmark without destination (a grouping heading). |
| `notes` | no | Exact text. |
| `tags` | no | Array of strings, order kept. |
| `state` | no | Storage state at export (`embedded`, `local_only`, `pending_embed`, `local_override`, `locally_hidden`, `conflict`): provenance only. Imported bookmarks are always Atlas's own until the reader saves them into the PDF. |
| `id` | no | Atlas ID at export. Imports always make new IDs. |
| `children` | no | Array of bookmarks under this one. |

Never in the file: passwords, file paths, database IDs.

## Versions

- **Same major (1.x.y):** imported. Members Atlas doesn't know are ignored; a `location` written by a newer Atlas (`atlas-location/2…`) is kept unchanged so a later Atlas can read it.
- **Newer major (2.0.0 and up):** refused with "This file comes from a newer Atlas Reader".
- **Not a version** (`"one"`, missing): refused.
- A minor version adds optional members only; anything that changes the meaning of an existing member is a new major.

## Refusals (the whole file, nothing half-imported)

| Reason | When |
|---|---|
| `notJson` | Not a JSON object. |
| `wrongFormat` | `format` is not `atlas-bookmarks`, or `bookmarks` is not an array. |
| `newerVersion` / `badVersion` | See Versions. |
| `tooLarge` | Over 64 MiB. |
| `tooMany` | More bookmarks than one book may hold (200,000). |
| `tooDeep` | Nested deeper than 256 levels. |
| `badItem` | A bookmark without a text `title`; a `location`, `notes`, `tags` or `children` of the wrong type; a damaged destination (e.g. a negative page); a title or notes beyond the overlay's limits. A schema-valid file can still be refused here (fixture `damaged-item`). |

## Import modes (Bookmarks panel ⋯ → Import Bookmarks…)

The preview shows how many bookmarks the file holds, the first titles, and a warning when it came from another book. Then:

- **At the end of the list** (top level).
- **Under the selected bookmark.**
- **At the end, skipping bookmarks already here**: a bookmark whose titles from the top down *and* destination equal one already in the book is skipped; a new bookmark under a skipped one goes under the existing one.
- **Instead of the current bookmarks**: the PDF's own are hidden (kept in the file until a save with removal), Atlas's own removed.

Every import is one Undo step and changes nothing in the PDF until the reader saves.

## Markdown and CSV (export only)

- **Markdown:** `# <book title>`, then a nested list `- <title> — p. <page>` (the book's own page labels when it has them), notes as `> ` quotes, tags as `` `tag` ``. Markdown punctuation in titles is escaped.
- **CSV:** UTF-8 with BOM, CRLF, every field quoted. Columns `level,path,title,page,state,notes,tags`: `path` is the titles above, joined with ` › `; `tags` joined with `;`. A cell starting with `= + - @`, tab or CR gets a leading `'` so spreadsheets show it as text.
