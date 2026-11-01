import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Application title bar (DESIGN.md §3): mark and place, then language,
// appearance and Open. The Command Center box joins it in N5.
Rectangle {
    id: bar

    signal openPdfRequested()
    signal libraryRequested()
    property string placeTitle: ""

    implicitHeight: Theme.titleBarHeight
    color: Theme.panel

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.lg
        anchors.rightMargin: Theme.md
        spacing: Theme.sm

        Image {
            source: "assets/branding/atlas-reader-icon.png"
            sourceSize.width: 28
            sourceSize.height: 28
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            fillMode: Image.PreserveAspectFit
            smooth: true
            Accessible.ignored: true
        }
        Label {
            text: I18n.t("app.name")
            color: Theme.ink
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(15 * I18n.sizeScale)
            font.weight: Font.DemiBold
        }
        Label {
            visible: bar.placeTitle.length > 0
            text: "·"
            color: Theme.muted
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            visible: bar.placeTitle.length > 0
            text: bar.placeTitle
            color: Theme.muted
            elide: Text.ElideMiddle
            font.family: I18n.contentFamily(text)
            font.pixelSize: 14
        }
        Item { Layout.fillWidth: bar.placeTitle.length === 0 }

        AtlasButton {
            kind: "quiet"
            iconName: "local_language"
            text: I18n.languages.find(l => l.code === I18n.language).name
            Accessible.name: I18n.t("title.language")
            onClicked: languageMenu.open()
            Menu {
                id: languageMenu
                y: parent.height
                Repeater {
                    model: I18n.languages
                    delegate: MenuItem {
                        required property var modelData
                        text: modelData.name
                        checkable: true
                        checked: I18n.language === modelData.code
                        font.family: modelData.code === "ur" ? "Noto Nastaliq Urdu" : ""
                        onTriggered: preferences.language = modelData.code
                    }
                }
            }
        }
        AtlasIconButton {
            iconName: Theme.dark ? "weather_sunny" : "weather_moon"
            text: I18n.t("title.appearance")
            onClicked: appearanceMenu.open()
            Menu {
                id: appearanceMenu
                y: parent.height
                MenuItem {
                    text: I18n.t("appearance.system"); checkable: true
                    checked: preferences.themeMode === "system"
                    onTriggered: preferences.themeMode = "system"
                }
                MenuItem {
                    text: I18n.t("appearance.light"); checkable: true
                    checked: preferences.themeMode === "light"
                    onTriggered: preferences.themeMode = "light"
                }
                MenuItem {
                    text: I18n.t("appearance.dark"); checkable: true
                    checked: preferences.themeMode === "dark"
                    onTriggered: preferences.themeMode = "dark"
                }
                MenuSeparator {}
                MenuItem {
                    text: I18n.t("appearance.touch"); checkable: true
                    checked: preferences.touchMode
                    onTriggered: preferences.touchMode = !preferences.touchMode
                }
            }
        }
        AtlasButton {
            kind: "primary"
            iconName: "folder_open"
            text: I18n.t("action.openPdf")
            Accessible.description: "Ctrl+O"
            onClicked: bar.openPdfRequested()
        }
    }
}
