import QtQuick
import QtQuick.Controls.Basic

// Radio button drawn with Theme tokens (16 px, like AtlasCheckBox), so option
// groups match the check boxes in every skin and direction.
RadioButton {
    id: control
    font.family: I18n.uiFamily
    spacing: Theme.sm
    // Explicit, so screen readers find the option even before Qt Quick
    // Controls registers its own accessibility role (found by the a11y dump).
    Accessible.role: Accessible.RadioButton
    Accessible.name: text
    Accessible.checkable: true
    Accessible.checked: checked
    Accessible.focusable: true
    Accessible.onPressAction: if (enabled) { checked = true; clicked() }

    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: width / 2
        color: Theme.raised
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.focusRing : control.checked ? Theme.accent : Theme.controlBorder
        Rectangle {
            anchors.centerIn: parent
            width: 8
            height: 8
            radius: 4
            visible: control.checked
            color: Theme.accent
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
