import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Foxit-style document tab bar (FOXIT_PARITY §1): 28 px, fixed 124 px tabs
// with 1 px dividers; the Library is the pinned first tab (Foxit's Start
// tab); the active tab takes the page colour and shows its close button; the
// overflow button lists every open tab.
Rectangle {
    id: bar

    implicitHeight: Theme.tabBarHeight + 1
    color: Theme.sunken

    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.chromeLine }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.chromeLine; z: -1 }

    component DocTab: AbstractButton {
        id: tabItem
        property bool active: false
        property bool closable: true
        property string iconName: "document"
        property string contentText: ""
        signal closeRequested()
        width: Theme.docTabWidth
        height: Theme.tabBarHeight
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.PageTab
        Accessible.name: contentText
        Accessible.selected: active
        Keys.onReturnPressed: clicked()
        Keys.onSpacePressed: clicked()
        signal contextMenuRequested()
        TapHandler { acceptedButtons: Qt.MiddleButton; onTapped: if (tabItem.closable) tabItem.closeRequested() }
        TapHandler { acceptedButtons: Qt.RightButton; onTapped: tabItem.contextMenuRequested() }
        Keys.onMenuPressed: contextMenuRequested()
        ToolTip.visible: hovered && tabLabel.truncated
        ToolTip.delay: 600
        ToolTip.text: contentText

        background: Rectangle {
            color: tabItem.active ? Theme.raised : tabItem.hovered ? Theme.hover : Theme.docTab
            Rectangle { // divider
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.line
            }
            Rectangle {
                anchors.fill: parent
                visible: tabItem.visualFocus
                color: "transparent"
                border.width: 2
                border.color: Theme.focusRing
            }
        }
        contentItem: RowLayout {
            spacing: 4
            Label {
                id: tabLabel
                Layout.leftMargin: 8
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: tabItem.contentText
                color: tabItem.active ? Theme.brand : Theme.ink
                elide: Text.ElideRight
                font.family: I18n.contentFamily(text) || I18n.uiFamily
                font.pixelSize: Math.round(12 * I18n.sizeScale)
            }
            AtlasIconButton {
                visible: tabItem.closable && (tabItem.active || tabItem.hovered)
                Layout.rightMargin: 3
                implicitWidth: 18
                implicitHeight: 18
                iconSize: 12
                cornerRadius: 2
                iconName: "dismiss"
                text: I18n.t("tab.close", tabItem.contentText)
                shortcutHint: "Ctrl+W"
                onClicked: tabItem.closeRequested()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        spacing: 0

        DocTab {
            Layout.preferredWidth: Theme.docTabWidth
            Layout.fillHeight: true
            active: readerController.activeIndex < 0
            closable: false
            contentText: I18n.t("nav.library")
            onClicked: readerController.showLibrary()
        }

        ListView {
            id: tabList
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            clip: true
            interactive: false
            model: readerController
            currentIndex: readerController.activeIndex
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
            delegate: DocTab {
                required property int index
                required property string title
                required property string status
                active: index === readerController.activeIndex
                contentText: status === "loading" ? title + "…" : title
                onClicked: readerController.activeIndex = index
                onCloseRequested: readerController.closeAt(index)
                onContextMenuRequested: { tabMenu.tabIndex = index; tabMenu.popup() }
            }
            Menu { // Foxit tab context menu
                id: tabMenu
                property int tabIndex: -1
                AtlasMenuItem { text: I18n.t("tab.closeThis"); onTriggered: readerController.closeAt(tabMenu.tabIndex) }
                AtlasMenuItem { text: I18n.t("tab.closeOthers"); enabled: tabList.count > 1; onTriggered: readerController.closeOthers(tabMenu.tabIndex) }
                AtlasMenuItem { text: I18n.t("tab.closeLeft"); enabled: tabMenu.tabIndex > 0; onTriggered: readerController.closeLeft(tabMenu.tabIndex) }
                AtlasMenuItem { text: I18n.t("tab.closeRight"); enabled: tabMenu.tabIndex < tabList.count - 1; onTriggered: readerController.closeRight(tabMenu.tabIndex) }
                AtlasMenuItem { text: I18n.t("tab.closeAll"); onTriggered: readerController.closeAll() }
                MenuSeparator {}
                AtlasMenuItem { text: I18n.t("tab.copyPath"); onTriggered: readerController.copyPathAt(tabMenu.tabIndex) }
                AtlasMenuItem { text: I18n.t("tab.showInFolder"); onTriggered: readerController.showInFolderAt(tabMenu.tabIndex) }
            }
            WheelHandler {
                onWheel: (event) => tabList.contentX = Math.max(0, Math.min(tabList.contentWidth - tabList.width,
                    tabList.contentX - event.angleDelta.y))
            }
        }

        AtlasIconButton {
            Layout.rightMargin: 4
            implicitWidth: 24
            implicitHeight: 24
            iconSize: 14
            cornerRadius: 2
            iconName: "chevron_down"
            text: I18n.t("tabs.all")
            onClicked: allTabs.popup(this, 0, height)
            Menu {
                id: allTabs
                AtlasMenuItem {
                    text: I18n.t("nav.library")
                    checkable: true
                    checked: readerController.activeIndex < 0
                    onTriggered: readerController.showLibrary()
                }
                Instantiator {
                    model: readerController
                    delegate: MenuItem {
                        required property int index
                        required property string title
                        text: title
                        checkable: true
                        checked: index === readerController.activeIndex
                        onTriggered: readerController.activeIndex = index
                    }
                    onObjectAdded: (index, object) => allTabs.insertItem(index + 1, object)
                    onObjectRemoved: (index, object) => allTabs.removeItem(object)
                }
            }
        }
    }
}
