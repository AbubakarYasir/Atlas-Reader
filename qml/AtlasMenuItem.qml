import QtQuick
import QtQuick.Controls.Basic

// Menu item with explicit accessibility, so screen readers reach every
// command even before Qt Quick Controls registers its own role (a11y dump).
MenuItem {
    Accessible.role: Accessible.MenuItem
    Accessible.name: text
    Accessible.checkable: checkable
    Accessible.checked: checked
    Accessible.focusable: true
    Accessible.onPressAction: if (enabled) triggered()
}
