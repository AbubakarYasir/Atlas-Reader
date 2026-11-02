import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl

// Check box drawn with Theme tokens so the box is visible in every skin.
CheckBox {
    id: control
    font.family: I18n.uiFamily
    spacing: Theme.sm

    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: 2
        color: control.checked ? Theme.accent : Theme.raised
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.focusRing : control.checked ? Theme.accent : Theme.controlBorder
        IconImage {
            anchors.centerIn: parent
            visible: control.checked
            source: Theme.icon("checkmark")
            sourceSize.width: 14
            sourceSize.height: 14
            color: Theme.accentText
        }
    }
    contentItem: Text {
        leftPadding: control.mirrored ? 0 : control.indicator.width + control.spacing
        rightPadding: control.mirrored ? control.indicator.width + control.spacing : 0
        text: control.text
        font: control.font
        color: control.enabled ? Theme.ink : Theme.muted
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
    }
}
