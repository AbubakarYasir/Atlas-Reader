import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Foxit-style File page (backstage, FOXIT_PARITY §1): a 210 px command list
// on the leading side; Open shows "Recent Documents" and "Computer". Items
// appear only when they work (Save, Print, Properties arrive with N5/N7).
Rectangle {
    id: stage

    property string page: "open"     // open | skins
    signal closeRequested()
    signal openFileRequested()
    signal preferencesRequested()
    signal exitRequested()

    color: Theme.raised
    Keys.onEscapePressed: closeRequested()

    component StageItem: AbstractButton {
        id: stageItem
        property bool selected: false
        Layout.fillWidth: true
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        implicitHeight: 31 + Theme.scriptExtra
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.MenuItem
        Accessible.name: text
        background: Rectangle {
            radius: 2
            color: stageItem.selected ? Theme.brand : stageItem.hovered ? Theme.hover : "transparent"
            border.width: stageItem.visualFocus ? 2 : 0
            border.color: Theme.focusRing
        }
        contentItem: Text {
            leftPadding: 5
            text: stageItem.text
            color: stageItem.selected ? Theme.accentText : stageItem.enabled ? Theme.ink : Theme.muted
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(14 * I18n.sizeScale)
            font.weight: stageItem.selected ? Font.DemiBold : Font.Normal
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }
    }
    component StageDivider: Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.topMargin: 6
        Layout.bottomMargin: 6
        implicitHeight: 1
        color: Theme.hover
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- command list
        Rectangle {
            Layout.preferredWidth: Theme.backstageListWidth - 1
            Layout.fillHeight: true
            color: Theme.rail
            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 12
                spacing: 2
                StageItem { text: I18n.t("backstage.open"); selected: stage.page === "open"; onClicked: stage.page = "open" }
                StageDivider {}
                StageItem {
                    text: I18n.t("backstage.close")
                    enabled: readerController.activeIndex >= 0
                    onClicked: { readerController.closeActive(); stage.closeRequested() }
                }
                StageItem {
                    text: I18n.t("backstage.closeAll")
                    enabled: readerController.hasSessions
                    onClicked: { readerController.closeAll(); stage.closeRequested() }
                }
                StageDivider {}
                StageItem { text: I18n.t("backstage.skins"); selected: stage.page === "skins"; onClicked: stage.page = "skins" }
                StageItem { text: I18n.t("backstage.preferences"); onClicked: stage.preferencesRequested() }
                StageDivider {}
                StageItem { text: I18n.t("backstage.exit"); onClicked: stage.exitRequested() }
                Item { Layout.fillHeight: true }
            }
        }
        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.chromeLine }

        // ---- Open
        RowLayout {
            visible: stage.page === "open"
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ColumnLayout {
                Layout.preferredWidth: 299
                Layout.fillHeight: true
                spacing: 4
                Label {
                    Layout.topMargin: 22
                    Layout.leftMargin: 24
                    Layout.bottomMargin: 18
                    text: I18n.t("backstage.open")
                    color: Theme.ink
                    font.pixelSize: Math.round(20 * I18n.sizeScale)
                }
                StageItem {
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    text: I18n.t("backstage.recent")
                    selected: false
                    background: Rectangle { radius: 2; color: Theme.hover }
                }
                StageItem {
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    text: I18n.t("backstage.computer")
                    onClicked: stage.openFileRequested()
                }
                Item { Layout.fillHeight: true }
            }
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.chromeLine }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0
                Label {
                    Layout.topMargin: 22
                    Layout.leftMargin: 28
                    Layout.bottomMargin: 16
                    text: I18n.t("backstage.recent")
                    color: Theme.ink
                    font.pixelSize: Math.round(20 * I18n.sizeScale)
                }
                Label {
                    Layout.leftMargin: 28
                    visible: recentList.count === 0
                    text: I18n.t("backstage.noRecent")
                    color: Theme.muted
                }
                ListView {
                    id: recentList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 28
                    Layout.rightMargin: 28
                    clip: true
                    model: preferences.recentFiles
                    ScrollBar.vertical: ScrollBar {}
                    delegate: AbstractButton {
                        id: row
                        required property string modelData
                        readonly property string fileName: modelData.replace(/\\/g, "/").split("/").pop()
                        width: ListView.view.width
                        height: 54 + Theme.scriptExtra
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        Accessible.role: Accessible.ListItem
                        Accessible.name: fileName
                        onClicked: { readerController.openLocalPath(modelData); stage.closeRequested() }
                        background: Rectangle {
                            color: row.hovered ? Theme.hover : "transparent"
                            border.width: row.visualFocus ? 2 : 0
                            border.color: Theme.focusRing
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hover }
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            IconImage {
                                Layout.leftMargin: 8
                                source: Theme.icon("document")
                                sourceSize.width: 24
                                sourceSize.height: 24
                                color: Theme.muted
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Label {
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignLeft
                                    text: row.fileName
                                    color: Theme.ink
                                    elide: Text.ElideRight
                                    font.family: I18n.contentFamily(text)
                                    font.pixelSize: 14
                                }
                                Label {
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.ltr(row.modelData)
                                    color: Theme.muted
                                    elide: Text.ElideMiddle
                                    font.family: ""
                                    font.pixelSize: 12
                                }
                            }
                            AtlasIconButton {
                                visible: row.hovered || activeFocus
                                implicitWidth: 26
                                implicitHeight: 26
                                iconSize: 14
                                iconName: "dismiss"
                                text: I18n.t("backstage.removeRecent")
                                onClicked: preferences.removeRecentFile(row.modelData)
                            }
                        }
                    }
                }
            }
        }

        // ---- Skins
        ScrollView {
            visible: stage.page === "skins"
            Layout.fillWidth: true
            Layout.fillHeight: true
            SkinsPanel {
                x: 28
                y: 22
                width: Math.min(560, stage.width - Theme.backstageListWidth - 56)
                showTitle: true
            }
        }
    }
}
