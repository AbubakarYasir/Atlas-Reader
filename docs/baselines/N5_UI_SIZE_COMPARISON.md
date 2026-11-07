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
| Bookmark menu width | no runtime capture in the package | 224 | 200 | 201 | Content-driven, 200–480 px |
| Bookmark menu row | no runtime capture in the package | 28 | 32 | 48 | Nastaliq needs the height (DESIGN.md: never shrink Urdu to Foxit's row) |
| Save bookmarks dialog | no Foxit equivalent | 412×144 | 412×154 | 412×261 | — |
| Export bookmarks dialog | no Foxit equivalent | 432×303 | 432×358 | 432×489 | — |
| About dialog | not compared | 412×360 | 412×387 | 412×592 | — |

Open: one runtime screenshot of Foxit's bookmark right-click menu at 100% (the
owner's PC) to compare menu width and row height.
