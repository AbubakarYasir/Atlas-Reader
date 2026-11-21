import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Skins (Foxit File > Skins): mode Light / Dark / Use system setting, and for
// Light mode a Classic or Colourful title bar with a theme colour. Changes
// apply at once; the Preferences window can restore its snapshot on Cancel.
ColumnLayout {
    id: panel
    property bool showTitle: false
    spacing: Theme.md

    Label {
        visible: panel.showTitle
        text: I18n.t("backstage.skins")
        color: Theme.ink
        font.pixelSize: Math.round(20 * I18n.sizeScale)
        Layout.bottomMargin: Theme.sm
    }

    GroupBox {
        Layout.fillWidth: true
        title: I18n.t("skins.mode")
        ColumnLayout {
            anchors.fill: parent
            spacing: 2
            Repeater {
                model: [
                    { mode: "light", key: "skins.light" },
                    { mode: "dark", key: "skins.dark" },
                    { mode: "system", key: "skins.system" }
                ]
                delegate: AtlasRadioButton {
                    required property var modelData
                    text: I18n.t(modelData.key)
                    checked: preferences.themeMode === modelData.mode
                    onClicked: preferences.themeMode = modelData.mode
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        title: I18n.t("skins.lightTheme")
        enabled: !preferences.dark
        ColumnLayout {
            anchors.fill: parent
            spacing: 2
            AtlasRadioButton {
                text: I18n.t("skins.classic")
                checked: preferences.skin === "classic"
                onClicked: preferences.skin = "classic"
            }
            AtlasRadioButton {
                id: colorfulButton
                text: I18n.t("skins.colorful")
                checked: preferences.skin === "colorful"
                onClicked: preferences.skin = "colorful"
            }
            Label {
                Layout.topMargin: Theme.xs
                text: I18n.t("skins.themeColor")
                color: Theme.muted
            }
            Flow {
                Layout.fillWidth: true
                spacing: Theme.sm
                Repeater {
                    model: Theme.themeColors.length
                    delegate: AbstractButton {
                        id: swatch
                        required property int index
                        width: 32
                        height: 32
                        enabled: preferences.skin === "colorful"
                        checkable: true
                        checked: preferences.themeColor === index
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: I18n.t("skins.color." + Theme.themeColorNames[index])
                        Accessible.checked: checked
                        ToolTip.visible: hovered
                        ToolTip.text: I18n.t("skins.color." + Theme.themeColorNames[index])
                        onClicked: preferences.themeColor = index
                        background: Rectangle {
                            radius: 4
                            color: Theme.themeColors[swatch.index]
                            opacity: swatch.enabled ? 1 : 0.35
                            border.width: swatch.checked || swatch.visualFocus ? 3 : 0
                            border.color: swatch.visualFocus ? Theme.focusRing : Theme.ink
                        }
                    }
                }
            }
        }
    }
}
