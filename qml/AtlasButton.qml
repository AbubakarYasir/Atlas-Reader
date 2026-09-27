import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control

    implicitWidth: Math.max(82, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 40
    leftPadding: 14
    rightPadding: 14
    topPadding: 9
    bottomPadding: 9
    spacing: 8
    hoverEnabled: true
    Accessible.name: text

    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled
            ? Qt.alpha(control.palette.buttonText, 0.42)
            : control.highlighted
                ? control.palette.highlightedText
                : control.palette.buttonText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 9
        color: {
            if (!control.enabled) return Qt.alpha(control.palette.button, 0.45)
            if (control.down) return Qt.darker(control.highlighted ? control.palette.highlight : control.palette.button, 1.12)
            if (control.highlighted) return control.palette.highlight
            if (control.flat && !control.hovered) return "transparent"
            if (control.hovered) return Qt.lighter(control.palette.button, 1.06)
            return control.palette.button
        }
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus
            ? control.palette.highlight
            : control.highlighted || control.flat
                ? "transparent"
                : Qt.alpha(control.palette.buttonText, 0.14)
    }
}
