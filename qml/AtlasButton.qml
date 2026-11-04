import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl

// Text button with optional leading icon. kind: "secondary" (default),
// "primary", "quiet" or "danger". docs/DESIGN.md §5.
Button {
    id: control

    property string kind: highlighted ? "primary" : (flat ? "quiet" : "secondary")
    property string iconName: ""
    property bool mirrorIcon: false
    property int textAlignment: Text.AlignHCenter

    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(Theme.controlHeight, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: text.length > 0 ? Theme.md : Theme.sm
    rightPadding: text.length > 0 ? Theme.md : Theme.sm
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.sm
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font.family: I18n.uiFamily
    font.pixelSize: Math.round(14 * I18n.sizeScale)
    Accessible.name: text

    icon.source: iconName.length > 0 ? Theme.icon(iconName) : ""
    icon.width: Theme.iconSize
    icon.height: Theme.iconSize
    icon.color: contentColor

    readonly property color contentColor: !enabled
        ? (Theme.highContrast ? Theme.ink : Qt.alpha(Theme.ink, 0.38))
        : kind === "primary" ? Theme.accentText
        : kind === "danger" ? Theme.danger
        : Theme.ink

    contentItem: Row {
        spacing: control.spacing
        layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight
        IconImage {
            visible: control.iconName.length > 0
            anchors.verticalCenter: parent.verticalCenter
            source: control.icon.source
            sourceSize.width: control.icon.width
            sourceSize.height: control.icon.height
            color: control.icon.color
            mirror: control.mirrorIcon && control.mirrored
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: control.text
            font: control.font
            color: control.contentColor
            elide: Text.ElideRight
            horizontalAlignment: control.textAlignment
            visible: text.length > 0
        }
    }

    background: Rectangle {
        implicitHeight: Theme.controlHeight
        radius: Theme.radiusControl
        color: {
            if (!control.enabled) return control.kind === "primary" ? Qt.alpha(Theme.accent, 0.35) : "transparent"
            if (control.kind === "primary") return control.down ? Qt.darker(Theme.accent, 1.12) : control.hovered ? Qt.lighter(Theme.accent, 1.08) : Theme.accent
            if (control.kind === "danger") return control.down ? Theme.pressed : control.hovered ? Theme.dangerSoft : "transparent"
            if (control.down) return Theme.pressed
            if (control.hovered || control.checked) return Theme.hover
            return control.kind === "quiet" ? "transparent" : Theme.raised
        }
        border.width: control.visualFocus ? 2 : Theme.highContrast ? 1 : (control.kind === "secondary" || control.kind === "danger" ? 1 : 0)
        border.color: control.visualFocus ? Theme.focusRing
            : control.kind === "danger" ? Theme.dangerBorder : Theme.line
        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }
}
