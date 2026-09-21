# Atlas Reader Native — UX, Accessibility, and Design Contract

## Product feel

Atlas should feel like a serious research instrument, not an office-suite toolbar wall and not a decorative consumer reader.

Default UI principles:

- content first;
- fast first interaction;
- one obvious primary action per state;
- advanced tools discoverable without crowding;
- stable layout while loading;
- keyboard parity with pointer actions;
- status/recovery language explains what happened and what is safe to do next;
- Arabic/RTL is a native layout mode, not translated LTR.

## 1. Desktop shell

Primary destinations:

- Library
- Recents
- Bookmarks
- Favorites
- Folders
- Settings

Document workspace:

- tabs for open books;
- persistent/collapsible navigation panel;
- reader canvas occupies most area;
- contextual reader/write controls instead of global permanent toolbars;
- dirty/pending/local-only state visible without dominating reading.

## 2. Reader workspace

Recommended navigation panel order:

1. Outline
2. Pages
3. Bookmarks
4. Annotations (once available)

The panel preserves selected page/current location and supports search appropriate to each tab.

Compact/narrow windows may move this panel into a drawer/sheet but must preserve the same information architecture and keyboard semantics.

## 3. Progressive disclosure

Default reader shows only high-frequency actions:

- navigation;
- zoom/fit;
- search;
- bookmark;
- write mode when implemented;
- reader settings.

Less frequent actions belong in contextual menus/sheets:

- document details;
- export/import;
- bookmark reconciliation;
- save working copy;
- advanced print options;
- diagnostics.

Do not expose every possible PDF function in a permanent ribbon.

## 4. State communication

Document/research data state must be understandable.

Examples:

- Embedded
- Local only
- Pending embed
- Conflict
- Signed/Certified
- Read-only
- Offline
- Locked

Use icon + text/tooltip/details; color is supplementary only.

Avoid generic messages such as “Error 32” or “Save failed” when Atlas knows the cause.

Preferred pattern:

> **This PDF is read-only. Your bookmark is saved locally.** Retry embedding after the file becomes writable, or save a working copy.

## 5. Dialog rules

Use confirmation dialogs only for genuinely destructive/irreversible decisions.

Prefer undo for ordinary reversible local actions.

Required preview dialogs:

- destructive bookmark import/replace;
- local/PDF reconciliation conflicts;
- migration/restore binding where identity is ambiguous;
- metadata save into PDF;
- signed/certified working-copy decision where needed.

Password prompts clearly distinguish opening the document from authorizing restricted modifications when the PDF model provides that distinction.

## 6. Keyboard model

Initial target map, refined per checkpoint:

| Action | Shortcut |
|---|---|
| Open PDF | `Ctrl+O` |
| Command Center | `Ctrl+K` |
| Find in document | `Ctrl+F` |
| Add bookmark | `Ctrl+B` |
| Save/commit current portable changes | `Ctrl+S` |
| Print | `Ctrl+P` |
| Close tab | `Ctrl+W` |
| Next/previous tab | `Ctrl+Tab` / `Ctrl+Shift+Tab` |
| Undo/redo | `Ctrl+Z` / `Ctrl+Shift+Z` |
| Rename focused bookmark | `F2` |
| Delete focused editable item | `Delete` |
| Activate focused item | `Enter` |
| Escape overlay/dialog | `Esc` |

Tree navigation uses conventional arrow-key semantics. Every context menu must be reachable through keyboard (`Shift+F10`/Menu key where supported).

Shortcuts are discoverable in tooltips/menus/settings and localizable by label, not by changing actual key definitions unpredictably.

## 7. Focus/accessibility

Every interactive custom control needs:

- accessible role;
- accessible name;
- state (selected/expanded/checked/disabled etc.);
- predictable focus order;
- visible focus indicator;
- keyboard activation;
- no pointer-only action.

Reader canvas controls must expose meaningful navigation/actions even though PDF page pixels themselves are not ordinary widgets.

Announcements should cover:

- save complete/failure;
- local-only fallback;
- sync/reconciliation result;
- search result count when useful;
- page jump/current page;
- import/export completion;
- offline/reconnected document state.

Avoid moving focus merely to announce status.

## 8. Scaling and layout

Windows qualification covers at least:

- 100% scale;
- 125/150% practical inspection;
- 200% scale acceptance;
- narrow and wide window sizes;
- multi-monitor/DPI transition later in qualification.

Text must wrap/reflow instead of silently clipping critical content.

Reader canvas and UI scaling are distinct: increasing UI text size must not unexpectedly change PDF zoom.

## 9. Arabic/RTL

Arabic locale mirrors structural UI direction where appropriate.

Rules:

- user content is not forcibly reversed;
- filenames/paths use bidi isolation around mixed segments;
- numbers/page labels remain readable and predictable;
- icons with directional meaning mirror; universal symbols do not mirror unnecessarily;
- tree indentation/hierarchy reads naturally in RTL;
- context menus/dialog button order follows Qt/platform locale conventions;
- Arabic, English, Urdu, punctuation and diacritics are tested together.

Search normalization policy must be explicit; never silently alter stored bookmark text merely to make search easier.

## 10. Touch and pen

Windows remains mouse/keyboard-first but architecture supports touch/pen.

- touch targets large enough for touch when touch mode is used;
- pan gesture never creates ink;
- stylus drawing uses low-latency live overlay;
- barrel/eraser behavior mapped only after hardware qualification;
- palm/touch interaction policy tested rather than guessed;
- user can always leave writing mode without losing unsaved work.

Android/iPad later may use a different responsive shell, but core commands and storage state remain the same.

## 11. Loading/empty/error states

### Loading

Show useful progress only when work is perceptible. Progressive scans should show books as batches complete rather than block on 100% completion.

### Empty

One explanation + one primary next action.

Examples:

- no library roots → Add folder / Open PDF;
- no bookmarks → Add bookmark;
- no search results → query guidance/clear filter.

### Error/restricted

Preserve unaffected capabilities. A read-only PDF should not look like a broken book if reading and local bookmarking still work.

## 12. Visual system

Use Qt Quick Controls/theme tokens rather than scattered literal colors/sizes.

Token groups should cover:

- surfaces/text;
- accent/focus;
- error/warning/success/info;
- local-only/pending/conflict state in combination with icons/text;
- spacing/radii;
- typography;
- reader background;
- touch target size;
- animation durations.

System light/dark/high-contrast behavior takes precedence over a rigid branded palette.

## 13. Motion

Animation exists to clarify state change, not decorate.

- panel transitions short and interruptible;
- no animation required before content becomes usable;
- honor reduced-motion/system accessibility preferences where available;
- never animate large PDF surfaces in a way that harms frame pacing.

## 14. Design review checklist

A user-facing change is reviewed for:

- Does the primary task remain obvious?
- Is the PDF canvas still the dominant reader surface?
- Can it be completed by keyboard?
- Does Narrator receive useful semantics?
- Does it work in Arabic/RTL?
- Does it work at 200%?
- Are local/portable states understandable?
- Is an error recoverable/actionable?
- Is advanced functionality hidden until needed rather than removed?
- Does the UI introduce synchronous work/frame instability?

Visual polish cannot override these requirements.