import QtQuick
import QtQuick.Controls.Basic

// Text field: accepts any language. Direction follows the content, not the UI.
TextField {
    id: control

    implicitHeight: Theme.controlHeight
    leftPadding: Theme.md
    rightPadding: Theme.md
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter
    hoverEnabled: true
    color: Theme.ink
    placeholderTextColor: Theme.muted
    selectionColor: Theme.accent
    selectedTextColor: Theme.accentText
    font.family: I18n.uiFamily
    font.pixelSize: Math.round(14 * I18n.sizeScale)

    background: Rectangle {
        radius: Theme.radiusControl
        color: Theme.raised
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? Theme.focusRing
            : control.hovered ? Theme.ink : Theme.controlBorder
    }
}
