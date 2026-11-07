import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Navigation row for side lists: icon, label, optional detail, selected state.
ItemDelegate {
    id: control

    property string iconName: ""
    property string detail: ""
    property bool selected: false

    Layout.fillWidth: true
    // Grows with the shaped text (Nastaliq lines are tall), never shorter than a control.
    topPadding: Theme.xs + Theme.scriptExtra / 2
    bottomPadding: Theme.xs + Theme.scriptExtra / 2
    implicitHeight: Math.max(Theme.controlHeight + 4, implicitContentHeight + topPadding + bottomPadding)
    leftPadding: Theme.sm
    rightPadding: Theme.sm
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    highlighted: selected
    Accessible.role: Accessible.ListItem
    Accessible.name: detail.length > 0 ? text + ", " + detail : text
    Accessible.selected: selected

    contentItem: RowLayout {
        spacing: Theme.sm
        IconImage {
            visible: control.iconName.length > 0
            source: control.iconName.length > 0 ? Theme.icon(control.iconName) : ""
            sourceSize.width: Theme.iconSize
            sourceSize.height: Theme.iconSize
            color: control.selected ? Theme.accent : Theme.muted
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            text: control.text
            color: Theme.ink
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(14 * I18n.sizeScale)
            font.weight: control.selected ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
        }
        Label {
            visible: control.detail.length > 0
            text: control.detail
            color: Theme.warning
            font.pixelSize: Math.round(12 * I18n.sizeScale)
        }
    }

    background: Rectangle {
        radius: Theme.radiusControl
        color: control.selected ? Theme.accentSoft : control.down ? Theme.pressed : control.hovered ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
        Rectangle {
            visible: control.selected
            width: 3
            radius: 1.5
            height: parent.height - Theme.md
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            color: Theme.accent
        }
    }
}
