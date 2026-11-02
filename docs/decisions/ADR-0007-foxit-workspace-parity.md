# ADR-0007 — Foxit workspace parity

**Status:** Accepted (owner direction 2026-11-01)
**Checkpoint:** N4.3 — Ribbon workspace, design system and three languages
**Supersedes:** the "concepts only, own layout" rule in PLAN §7 and the slim command bar of DESIGN §3 (2026-11-01 morning)

## Context

After seeing the first N4.3 revamp, the owner asked for Atlas to copy the UI and UX of Foxit PDF Editor "as much as possible", with exact measurements and the same buttons, because a custom minimal layout risks looking like a generic AI-generated app, and Atlas's users already know Foxit. The owner also asked that Foxit's feature set be planned into Atlas: necessary features before 2.0, the rest after 2.0.

## Decision

1. **Layout parity.** Atlas reproduces Foxit's ribbon-mode workspace: custom title bar with Quick Access Toolbar and file name, ribbon tab row (File + task tabs), 72 px command area with large and small buttons in groups, collapsible ribbon (Ctrl+F11), fixed-width document tabs with a first "start" tab (Atlas: **Library**), left navigation rail with panels (F4), document pane, status bar, File backstage page, Preferences window with a searchable category list, Skins (Light/Dark/System, Classic or Colourful light theme) and Mouse/Touch mode. Measurements, order and labels follow [FOXIT_PARITY.md](../FOXIT_PARITY.md).
2. **Keyboard parity.** Foxit's default shortcuts are adopted for every command Atlas has, including where they replace earlier Atlas keys (Ctrl+K becomes Preferences).
3. **Feature parity is staged.** Every Foxit command is classified in FOXIT_PARITY §3 as done, a pre-2.0 stage, or an after-2.0 track (A–K); all are planned (amended 2026-11-01). Tabs and groups appear only when their commands work; there are no greyed-out promises.
4. **Own assets, own brand.** Atlas does not ship, trace or reuse Foxit's icons, logos, skin images, manual text or any file from its installation, and does not use Foxit's purple. The owner's reference package is used for measurements, colour values of neutral roles, and the feature/menu/dialog inventory. Icons come from Fluent UI System Icons (MIT) at Foxit's sizes (16/20/24 px); the brand colour comes from the Atlas logo.
5. **Languages.** The workspace is fully translated and mirrored for Arabic and Urdu ([ADR-0006](ADR-0006-interface-languages.md)); Foxit offers neither.
6. **Custom title bar** keeps native Windows behaviour through the system move and resize loops (`startSystemMove`/`startSystemResize`: drag-to-edge snap, Win+arrow keys, resize edges), double-click to maximize and named buttons for Narrator. The Windows 11 Snap Layouts flyout on the maximize button needs a native hit-test and is tracked for N10; `--native-frame` keeps the system frame as a fallback.

## Alternatives considered

- **Concepts only with a slim command bar** (previous decision). Rejected by the owner: too custom, risked a generic look.
- **Extracting Foxit's icon and skin resources for a pixel-identical result.** Rejected: copyright and licence (reverse-engineering clause) risk, and it would ship another company's brand inside Atlas.
- **Qt Widgets ribbon libraries.** Rejected: Atlas's UI is Qt Quick; mixing toolkits costs accessibility and RTL quality.

## Consequences

- N4.3 grows: title bar, ribbon, backstage, Preferences window and skins become part of it. The library becomes the pinned first document tab.
- Every later stage adds its commands to the ribbon tab Foxit uses for them, and its panel to the rail.
- **Amended 2026-11-01 (owner):** every Foxit feature is planned (FOXIT_PARITY §3, AFTER_2_0 tracks A–K); connected features are opt-in. Only bypasses, cracking and hidden data collection are never built.
- Atlas's own colour comes from its logo (Atlas Blue/navy/cyan); Foxit's neutral greys are used for the chrome.
- Comparisons with Foxit at each stage review now include a side-by-side screenshot at the same window size.

## Revisit conditions

The owner changes direction; or a Foxit layout choice conflicts with accessibility (WCAG 2.2 AA, Narrator) or Arabic/Urdu quality, in which case accessibility and language quality win and the deviation is recorded in FOXIT_PARITY.md.
