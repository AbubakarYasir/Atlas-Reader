# ADR-0006 — Interface languages, string tables and bundled fonts

**Status:** Accepted (owner direction 2026-11-01)
**Checkpoint:** N4.3 — Workspace revamp, design system and three languages

## Context

The owner requires the first version of Atlas to ship a complete interface in **English (default), Arabic and Urdu**, switchable at any time, while every piece of user content (bookmarks, notes, tags, annotations, search) accepts **any language**. Until N4.2 the app had English/Arabic strings inlined in QML as `arabic ? "…" : "…"` expressions, which cannot scale to three languages and cannot be checked for completeness. Urdu is normally read in Nastaliq, which Windows does not ship by default.

## Decision

1. **String tables, not Qt Linguist.** All interface strings live in `qml/i18n/Strings.js`, one table per language (`en`, `ar`, `ur`) with the same keys. A QML singleton `I18n` exposes `language`, `rtl` and `t(key, …args)` (with `%1`… placeholders). Bindings that call `I18n.t()` re-evaluate when `language` changes, so switching is instant without restarting or reloading QML.
2. **Completeness is enforced in CI** by `tools/i18n/check-strings.py`: every key must exist and be non-empty in all three languages, placeholders must match, and no QML view may contain a quoted user-facing literal outside the tables (allow-list for symbols and test-only text).
3. **Language choice** is stored in the user profile (`preferences.ini`), defaults to English, and can be overridden with `--language en|ar|ur`.
4. **Fonts:** Noto Nastaliq Urdu (Urdu UI and content) and Noto Naskh Arabic (Arabic content) are bundled under the OFL with pinned SHA-256 (`assets/fonts/README.md`). Latin and Arabic interface chrome use Segoe UI. Other scripts in user content fall back to Windows system fonts.
5. **User content is never translated or normalized on save**; only search indexes normalize (documented per script in N4.5/N5).

## Alternatives considered

- **Qt Linguist (`qsTr` + `.ts`/`.qm` + `qt_add_translations`).** Standard and translator-friendly, but adds a build-tool dependency to CI and the local toolchain, needs `QQmlEngine::retranslate()` plumbing for live switching, and makes "every key in every language" harder to gate for a three-language, agent-maintained project. Revisit if external translators or many more languages arrive; migration is mechanical because keys are stable.
- **Only English + Arabic in 2.0.** Rejected by the owner.
- **Rely on system fonts for Urdu.** Windows lacks a Nastaliq font by default; Urdu in Naskh looks foreign to Urdu readers.

## Consequences

- Every new or changed string must be written in all three languages in the same commit; CI blocks otherwise.
- Translations are reviewed by the owner in the stage review (Urdu and Arabic quality is a release criterion, see N9).
- About 1.2 MB of fonts are added to the package; licences travel with the notices.

## Revisit conditions

More than three interface languages, community translators, or a need for plural rules beyond what `%n` handling in `I18n` covers.

## Related

`docs/DESIGN.md` §4.2, §9, §11 · `PLAN.md` §4.7 and decision log · `CHECKPOINTS.md` N4.3, N9 · `assets/fonts/README.md`
