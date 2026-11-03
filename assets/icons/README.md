# Icons

Outline icons from **Fluent UI System Icons** (Microsoft, MIT licence; see [LICENSE.txt](LICENSE.txt)), 20 px "regular" style, plus `star_filled.svg` (20 px "filled").

- Source: <https://github.com/microsoft/fluentui-system-icons>
- Pinned commit: `9eb184e96268d3293462f9d9d19225b4c7b07774` (fetched 2026-11-01), path `assets/<Name>/SVG/ic_fluent_<name>_20_regular.svg`, renamed to `<name>.svg`.
- Files are unmodified. They are tinted at runtime through the control's `icon.color`, so one file serves every theme.
- N5.3 added `bookmark_add`, `delete`, `edit`, `eye` and `eye_off` from the same commit for the Bookmarks panel, then `signature`, `arrow_undo` and `arrow_redo` for the Signatures panel and the Quick Access Undo/Redo.
- Add a new icon only from the same pinned commit (or update the pin for all of them) and list why in the CHANGELOG. Do not mix icon families.
