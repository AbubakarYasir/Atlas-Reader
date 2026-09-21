# Migration from the Flutter Atlas Reader

The native repository is a successor, not an in-place language conversion.

## Migration principles

1. Preserve user data before preserving implementation details.
2. PDFs are the best portable bridge for embedded outlines/standard annotations.
3. App-local Flutter data is migrated only through a versioned, tested converter.
4. The old application remains usable until native behavior is accepted.
5. Never mutate a user's only PDF copy merely to migrate it.
6. Never attach old local research data to a native document based only on filename.

## What can migrate naturally

When already embedded and interoperable:

- PDF outline/bookmarks;
- standard PDF annotations/ink;
- document metadata.

Native Atlas should rediscover these through normal indexing rather than special migration code.

## What needs explicit migration

Potential Flutter-local data:

- descriptions/tags;
- favorites;
- reading progress;
- registered roots;
- tab/session preferences;
- any local-only bookmark state introduced in later Flutter builds.

A migration tool must understand the old schema version and map data through guarded document identity.

## Strategy

### Phase A — side-by-side

Native uses its own profile/database and does not touch Flutter profile.

### Phase B — read-only migration preview

A tool reads a copied/backup Flutter database, reports recognized schema/version, lists documents and confidence of identity mapping, and shows what would migrate.

### Phase C — import

User confirms exact/ambiguous mappings. Native writes only its own database. Re-running the import is idempotent or explicitly names duplicates.

### Phase D — validation

Compare counts and sampled exact bookmark/favorite/progress states. Keep an export/report of the migration.

## Not allowed

- opening the live Flutter DB for destructive writes;
- assuming absolute Windows paths remain valid on Android/macOS/iOS;
- filename-only matching;
- rewriting source PDFs to add native IDs during migration without explicit user action;
- claiming migration complete while unsupported local fields are silently dropped.

## Version coexistence

The two apps should have distinct application identifiers/profile directories until migration is accepted. "Atlas Reader Native" is a development identity; product branding can converge later without sharing unstable storage directories.