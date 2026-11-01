import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl

// Small ribbon command (Foxit: 16 px icon in a 24 × 24 target, stacked two or
// three to a column). The label is the tooltip and the accessible name.
AbstractButton {
    id: control

    property string iconName: ""
    property string shortcutHint: ""

    implicitWidth: 26 + (Theme.touch ? 8 : 0)
    implicitHeight: 24 + (Theme.touch ? 8 : 0)
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.role: checkable ? Accessible.CheckBox : Accessible.Button
    Accessible.name: text
    Accessible.checked: checked
    Accessible.description: shortcutHint

    ToolTip.visible: hovered || visualFocus
    ToolTip.delay: 500
    ToolTip.text: shortcutHint.length > 0 ? text + "  (" + shortcutHint + ")" : text

    background: Rectangle {
        radius: 2
        color: !control.enabled ? "transparent"
            : control.down || control.checked ? Theme.pressed
            : control.hovered ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
    }
    contentItem: Item {
        IconImage {
            anchors.centerIn: parent
            source: control.iconName.length > 0 ? Theme.icon(control.iconName) : ""
            sourceSize.width: 16
            sourceSize.height: 16
            color: !control.enabled ? Qt.alpha(Theme.ink, 0.38) : control.checked ? Theme.accent : Theme.ink
        }
    }
}
