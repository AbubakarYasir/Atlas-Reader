import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control

    implicitHeight: 42
    leftPadding: 13
    rightPadding: 13
    topPadding: 9
    bottomPadding: 9
    hoverEnabled: true

    background: Rectangle {
        radius: 8
        color: control.palette.base
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus
            ? control.palette.highlight
            : control.hovered
                ? Qt.alpha(control.palette.buttonText, 0.34)
                : Qt.alpha(control.palette.buttonText, 0.20)
    }
}
