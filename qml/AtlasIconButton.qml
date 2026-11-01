import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl

// Icon-only button. `text` is required: it becomes the tooltip and the
// accessible name, so no icon is ever unlabeled. docs/DESIGN.md §5.
Button {
    id: control

    property string iconName: ""
    property bool mirrorIcon: false
    property string shortcutHint: ""

    implicitWidth: Theme.iconButton
    implicitHeight: Theme.iconButton
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    display: AbstractButton.IconOnly
    Accessible.name: text
    Accessible.description: shortcutHint

    ToolTip.visible: (hovered || visualFocus) && text.length > 0
    ToolTip.delay: 500
    ToolTip.text: shortcutHint.length > 0 ? text + "  (" + shortcutHint + ")" : text

    contentItem: IconImage {
        source: control.iconName.length > 0 ? Theme.icon(control.iconName) : ""
        sourceSize.width: Theme.iconSize
        sourceSize.height: Theme.iconSize
        fillMode: Image.Pad
        color: !control.enabled ? Qt.alpha(Theme.ink, 0.38)
            : control.checked ? Theme.accent : Theme.ink
        mirror: control.mirrorIcon && control.mirrored
    }

    background: Rectangle {
        radius: Theme.radiusControl
        color: control.down ? Theme.pressed
            : control.checked ? Theme.accentSoft
            : control.hovered ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }
}
