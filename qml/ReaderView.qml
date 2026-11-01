import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Reader: document tab bar plus one workspace per open tab.
Item {
    id: reader

    signal openPdfRequested()
    readonly property var currentTab: tabStack.currentTab

    function cycleTab(step) {
        if (tabList.count === 0) return
        const current = readerController.activeIndex
        readerController.activeIndex = current < 0 ? 0 : (current + step + tabList.count) % tabList.count
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- document tab bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.tabBarHeight
            visible: !AppState.chromeHidden
            color: Theme.sunken
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sm
                anchors.rightMargin: Theme.sm
                spacing: Theme.xs

                AtlasButton {
                    kind: "quiet"
                    iconName: "library"
                    text: I18n.t("nav.library")
                    implicitHeight: Theme.tabBarHeight - 8
                    Accessible.name: I18n.t("nav.returnToLibrary")
                    onClicked: readerController.showLibrary()
                }
                Rectangle { width: 1; Layout.preferredHeight: Theme.tabBarHeight - 16; color: Theme.line }

                ListView {
                    id: tabList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    spacing: 2
                    clip: true
                    model: readerController
                    currentIndex: readerController.activeIndex
                    boundsBehavior: Flickable.StopAtBounds
                    onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)

                    delegate: Rectangle {
                        id: tab
                        required property int index
                        required property string title
                        required property string status
                        readonly property bool active: index === readerController.activeIndex
                        width: Math.min(240, Math.max(140, tabTitle.implicitWidth + 84))
                        height: tabList.height
                        color: active ? Theme.canvas : tabHover.hovered ? Theme.hover : "transparent"
                        radius: Theme.radiusControl
                        activeFocusOnTab: true
                        Accessible.role: Accessible.PageTab
                        Accessible.name: title
                        Accessible.focusable: true
                        Accessible.selected: active
                        Accessible.onPressAction: readerController.activeIndex = index
                        Keys.onReturnPressed: readerController.activeIndex = index
                        Keys.onSpacePressed: readerController.activeIndex = index
                        HoverHandler { id: tabHover }
                        TapHandler { onTapped: readerController.activeIndex = tab.index }
                        TapHandler { acceptedButtons: Qt.MiddleButton; onTapped: readerController.closeAt(tab.index) }

                        Rectangle { // active indicator
                            visible: tab.active
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - Theme.lg
                            height: 2
                            radius: 1
                            color: Theme.accent
                        }
                        Rectangle {
                            anchors.fill: parent
                            visible: tab.activeFocus
                            color: "transparent"
                            radius: Theme.radiusControl
                            border.width: 2
                            border.color: Theme.focusRing
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.md
                            anchors.rightMargin: Theme.xs
                            spacing: Theme.xs
                            IconImage {
                                source: Theme.icon(tab.status === "passwordRequired" ? "lock_closed" : "document")
                                sourceSize.width: 16
                                sourceSize.height: 16
                                color: tab.active ? Theme.accent : Theme.muted
                            }
                            Label {
                                id: tabTitle
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: tab.status === "loading" ? tab.title + "…" : tab.title
                                color: tab.active ? Theme.ink : Theme.muted
                                font.family: I18n.contentFamily(tab.title)
                                font.weight: tab.active ? Font.DemiBold : Font.Normal
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                            AtlasIconButton {
                                implicitWidth: 26
                                implicitHeight: 26
                                iconName: "dismiss"
                                text: I18n.t("tab.close", tab.title)
                                shortcutHint: "Ctrl+W"
                                opacity: tab.active || tabHover.hovered || activeFocus ? 1 : 0
                                onClicked: readerController.closeAt(tab.index)
                            }
                        }
                    }
                }

                AtlasIconButton {
                    iconName: "add"
                    text: I18n.t("action.openAnother")
                    shortcutHint: "Ctrl+O"
                    onClicked: reader.openPdfRequested()
                }
            }
        }

        // ---- workspaces
        Item {
            id: tabStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            property var currentTab: null

            Repeater {
                model: readerController
                delegate: ReaderTab {
                    anchors.fill: parent
                    visible: index === readerController.activeIndex
                    onVisibleChanged: if (visible) tabStack.currentTab = this
                    Component.onCompleted: if (visible) tabStack.currentTab = this
                }
            }
        }
    }
}
