# Atlas Reader Native — Agent Rules

Read this before changing the repository.

## Product priority

Work in the order **Index → Reader → Bookmarks** unless the active checkpoint says otherwise. Do not expand scope because the new stack makes an unrelated feature attractive.

## Architecture rules

- Shared domain/core code is C++23 and platform-neutral.
- Do not introduce Win32, QML, Java/Kotlin, Objective-C/Swift, or GUI types into core domain contracts.
- QML is presentation. Do not implement reconciliation, PDF security logic, identity matching, database rules, or save policy in QML/JavaScript.
- Platform behavior goes behind adapters.
- PDF behavior goes behind `IPdfEngine`/related facades. Never bind application logic directly to one PDF vendor API.
- Database access goes behind repositories/services; UI code never emits arbitrary SQL.

## Performance rules

Before adding background/document work, read `docs/PERFORMANCE.md`.

Never synchronously perform directory traversal, PDF parse/render/save, hashing, cover generation, large SQL work, or large reconciliation on the UI/render thread.

Do not claim "zero lag". Measure named scenarios and publish frame/startup/search/render/ink metrics.

## Document safety rules

Before touching save/security/bookmark behavior, read `docs/CORE_WORKFLOWS.md`.

- Never bypass PDF security/password restrictions.
- Never silently mutate a signed/certified original when doing so may affect integrity.
- Never blindly overwrite a document that changed outside Atlas.
- Never report local work as embedded before validated commit/reopen.
- An unavailable library root is not evidence that all its books were deleted.
- Ambiguous document identity must not steal another copy's research data.

## Accessibility/i18n rules

User-facing changes must preserve Arabic/RTL, mixed-script text, keyboard access, screen-reader semantics, focus visibility, 200% scaling, and non-color-only states.

## Documentation rules

Keep README, PLAN, CHECKPOINTS, INFO, CHANGELOG, and relevant docs consistent.

Use these meanings precisely:

- **planned** — documented requirement only;
- **implemented** — code exists;
- **verified** — automated/manual evidence exists;
- **accepted** — owner explicitly passed the checkpoint.

Never upgrade a status just because code compiles.

## Migration rule

The Flutter repository is a behavioral reference. Do not copy its widget architecture or reproduce known performance problems mechanically. Port contracts, fixtures, and user-visible behavior deliberately.

## Secrets

Never inspect, print, commit, or request secrets unnecessarily. Password-protected PDF fixture passwords must be test-only values, never personal document passwords.

## Current stop gate

N0 is bootstrap only. Do not implement PDF, SQLite, scanner, reader, or bookmark features until N0 is accepted and N1 begins.