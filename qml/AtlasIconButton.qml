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
    property int iconSize: Theme.iconSize
    property color iconColor: Theme.ink
    property color hoverColor: Theme.hover
    property int cornerRadius: Theme.radiusControl

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
        sourceSize.width: control.iconSize
        sourceSize.height: control.iconSize
        fillMode: Image.Pad
        color: !control.enabled ? Qt.alpha(Theme.ink, 0.38)
            : control.checked ? Theme.accent : control.iconColor
        mirror: control.mirrorIcon && control.mirrored
    }

    background: Rectangle {
        radius: control.cornerRadius
        color: control.down ? Qt.darker(control.hoverColor, 1.08)
            : control.checked ? Theme.accentSoft
            : control.hovered ? control.hoverColor : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }
}
