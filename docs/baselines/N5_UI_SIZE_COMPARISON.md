# N5 UI size comparison with Foxit (UI-SIZE-01)

<!-- atlas-status: N5|in-progress -->

Atlas sizes come from the accessibility dump (`--show a11y:<surface>@<file>`,
on-screen rectangles, 100% scale, 1280×820 window, Linux Qt 6.4 scratch build;
the Windows build uses the same QML). Foxit sizes come from the owner's reference
package (`foxit-ui-ux-reference-2026.1.1.36485`): runtime screenshots where they
exist, otherwise Foxit's own `.ui` layout sizes (design units, not proven pixels).

| Surface | Foxit | Atlas EN | Atlas AR | Atlas UR | Verdict |
|---|---|---|---|---|---|
| Preferences dialog | 770×548 (runtime, `preferences-general-light.png`) | 780×540 | 780×540 | 780×540 | Matches |
| Auto Create Bookmarks | 707×521 (`CreateBookmarkDlg.ui`, design units) | 512×552 | 512×552 | 512×552 | Different on purpose: one preview list instead of Foxit's options pane and level table |
| Bookmark menu width | 235 (Foxit PDF Reader 2026.2.1 runtime, 100 %) | 224 | 200 | 202 | Matches: content-driven, 200–480 px |
| Bookmark menu row | 23 (same capture), separators ~9 | 25 | 32 | 48 | English matched to the 24 px WCAG 2.2 target minimum (one px above Foxit); Arabic and Urdu grow with their text (DESIGN.md: never shrink Urdu to Foxit's row) |
| Other menus (tabs, ribbon, find) | — | 25 (was Basic's 40) | 32 | 48 | Same density as the bookmark menu since 2026-11-08 |
| Save bookmarks dialog | no Foxit equivalent | 412×144 | 412×154 | 412×261 | — |
| Export bookmarks dialog | no Foxit equivalent | 432×303 | 432×358 | 432×489 | — |
| About dialog | not compared | 412×360 | 412×387 | 412×592 | — |

Foxit menu reference: `.github/workflows/foxit-reference-capture.yml` installs the
free Foxit PDF Reader on a disposable Windows runner, opens A003, right-clicks
the first bookmark and keeps the screenshot as a CI artifact (2026-11-08: five
commands, 235×125 px, rows 23 px apart). Measurement only; nothing from Foxit
is in the repository. Foxit PDF Editor's menu (more commands) was not
captured.
