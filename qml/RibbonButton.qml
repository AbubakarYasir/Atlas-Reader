import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl

// Large ribbon command (Foxit layout, FOXIT_PARITY §1): 24 px icon with 6 px
// top padding over a 12 px label of one or two lines; "▾" marks a menu.
// Two-word labels break onto two lines like Foxit ("Page Fit / Option ▾").
AbstractButton {
    id: control

    property string iconName: ""
    property Menu menu: null
    property string shortcutHint: ""
    readonly property bool hasMenu: menu !== null

    // Break at the space nearest the middle so the button stays narrow.
    readonly property var lines: {
        const words = text.split(" ")
        // Nastaliq lines are too tall for two rows inside the ribbon.
        if (words.length < 2 || I18n.language === "ur") return [text, ""]
        let best = 1, bestDiff = 1e9
        for (let i = 1; i < words.length; ++i) {
            const a = words.slice(0, i).join(" ").length
            const b = words.slice(i).join(" ").length
            if (Math.abs(a - b) < bestDiff) { bestDiff = Math.abs(a - b); best = i }
        }
        return [words.slice(0, best).join(" "), words.slice(best).join(" ")]
    }

    implicitHeight: Theme.ribbonHeight
    implicitWidth: Math.max(42, Math.max(line1.implicitWidth, line2.visible ? line2.implicitWidth : 0) + 16)
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.role: hasMenu ? Accessible.ButtonMenu : Accessible.Button
    Accessible.name: text
    Accessible.description: shortcutHint

    ToolTip.visible: hovered && !pressed && (shortcutHint.length > 0)
    ToolTip.delay: 600
    ToolTip.text: text + "  (" + shortcutHint + ")"

    onClicked: if (hasMenu) menu.popup(control, 0, control.height)

    background: Rectangle {
        radius: 2
        color: !control.enabled ? "transparent"
            : control.down || control.checked || (control.hasMenu && control.menu.visible) ? Theme.pressed
            : control.hovered ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
    }

    contentItem: Item {
        IconImage {
            id: glyph
            anchors.horizontalCenter: parent.horizontalCenter
            y: 6
            source: control.iconName.length > 0 ? Theme.icon(control.iconName) : ""
            sourceSize.width: 24
            sourceSize.height: 24
            color: control.enabled ? Theme.ink : Qt.alpha(Theme.ink, 0.38)
        }
        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 6 + 24 + 6
            spacing: -2
            Text {
                id: line1
                anchors.horizontalCenter: parent.horizontalCenter
                text: control.lines[0] + (control.hasMenu && control.lines[1].length === 0 ? " ▾" : "")
                font.family: I18n.uiFamily
                font.pixelSize: Math.round(12 * I18n.sizeScale)
                color: control.enabled ? Theme.ink : Qt.alpha(Theme.ink, 0.38)
            }
            Text {
                id: line2
                anchors.horizontalCenter: parent.horizontalCenter
                visible: control.lines[1].length > 0
                text: control.lines[1] + (control.hasMenu ? " ▾" : "")
                font: line1.font
                color: line1.color
            }
        }
    }
}
