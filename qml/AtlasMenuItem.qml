import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl

// Menu item with explicit accessibility, so screen readers reach every
// command even before Qt Quick Controls registers its own role (a11y dump).
// One density for every Atlas menu: Foxit's (measured 23 px rows at 100 %,
// docs/baselines/N5_UI_SIZE_COMPARISON.md) raised to the 24 px WCAG 2.2
// target minimum; rows grow with their text (Arabic, Urdu, large text) and
// never crop glyphs. Ignores Basic's fixed 40 px background minimum.
MenuItem {
    id: control
    topPadding: Theme.xs
    bottomPadding: Theme.xs
    leftPadding: Theme.sm
    rightPadding: Theme.sm
    implicitHeight: Math.max(Theme.menuRowMinimum,
        implicitContentHeight + topPadding + bottomPadding,
        implicitIndicatorHeight + topPadding + bottomPadding)
    implicitWidth: Math.max(Theme.menuMinimumWidth,
        implicitContentWidth + leftPadding + rightPadding)
    // A 16 px check mark (Basic's 28 px box made checkable rows taller).
    indicator: Item {
        implicitWidth: 16
        implicitHeight: 16
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checkable
        IconImage {
            anchors.centerIn: parent
            visible: control.checked
            source: Theme.icon("checkmark")
            sourceSize.width: 14
            sourceSize.height: 14
            color: control.enabled ? Theme.ink : Theme.muted
        }
    }
    Accessible.role: Accessible.MenuItem
    Accessible.name: text
    Accessible.checkable: checkable
    Accessible.checked: checked
    Accessible.focusable: true
    Accessible.onPressAction: if (enabled) triggered()
}
