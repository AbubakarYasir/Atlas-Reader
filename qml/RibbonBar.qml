import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Atlas.Reader

// Foxit-style ribbon (FOXIT_PARITY §1, §3): a 28 px tab row (File + task
// tabs) over a 72 px command area of grouped buttons. Tabs appear only when
// their commands work; commands that need a document are disabled in the
// Library. Ctrl+F11 or the chevron collapses the command area.
Rectangle {
    id: ribbon

    property var viewport: null          // active ReaderViewport or null
    property int currentTab: 1           // 0 File, 1 Home, 2 View, 3 Accessibility, 4 Help
    property bool backstageOpen: false
    property bool peek: false            // collapsed ribbon shown temporarily
    readonly property bool hasDocument: viewport !== null
    readonly property bool collapsed: preferences.ribbonCollapsed
    readonly property bool showCommands: !collapsed || peek

    signal fileRequested()
    signal aboutRequested()
    signal preferencesRequested(string page)

    implicitHeight: Theme.ribbonTabHeight + (showCommands && !backstageOpen ? Theme.ribbonHeight + 5 : 1)
    color: Theme.chrome

    readonly property var tabs: [
        { key: "ribbon.file" }, { key: "ribbon.home" }, { key: "ribbon.view" },
        { key: "ribbon.accessibility" }, { key: "ribbon.help" }
    ]

    function zoomTo(factor) { if (viewport) viewport.zoom = factor }

    // ---- tab row
    Row {
        id: tabRow
        height: Theme.ribbonTabHeight
        anchors.left: parent.left
        anchors.leftMargin: 0
        Repeater {
            model: ribbon.tabs
            delegate: AbstractButton {
                id: tabButton
                required property int index
                required property var modelData
                readonly property bool selected: index === 0 ? ribbon.backstageOpen
                    : !ribbon.backstageOpen && ribbon.showCommands && ribbon.currentTab === index
                height: Theme.ribbonTabHeight
                width: label.implicitWidth + 32
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                Accessible.role: Accessible.PageTab
                Accessible.name: label.text
                Accessible.selected: selected
                onClicked: {
                    if (index === 0) { ribbon.fileRequested(); return }
                    if (ribbon.collapsed) ribbon.peek = !(ribbon.peek && ribbon.currentTab === index)
                    ribbon.currentTab = index
                }
                onDoubleClicked: if (index > 0) preferences.ribbonCollapsed = !preferences.ribbonCollapsed
                background: Rectangle {
                    color: tabButton.hovered && !tabButton.selected ? Theme.hover : "transparent"
                    border.width: tabButton.visualFocus ? 2 : 0
                    border.color: Theme.focusRing
                }
                contentItem: Item {
                    Text {
                        id: label
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -1
                        text: I18n.t(tabButton.modelData.key)
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(14 * I18n.sizeScale)
                        font.weight: tabButton.selected ? Font.DemiBold : Font.Normal
                        color: tabButton.selected ? Theme.brand : Theme.muted
                    }
                    Rectangle { // 3 px underline the width of the text (Foxit)
                        visible: tabButton.selected
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 4
                        width: label.implicitWidth
                        height: 3
                        color: Theme.brand
                    }
                }
            }
        }
    }

    // ---- command area
    Item {
        id: commands
        visible: ribbon.showCommands && !ribbon.backstageOpen
        anchors.top: tabRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: Theme.ribbonHeight

        component Separator: Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            Layout.topMargin: 2
            Layout.bottomMargin: 2
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            color: Theme.spacing
        }

        // Home
        RowLayout {
            visible: ribbon.currentTab === 1
            anchors.left: parent.left
            anchors.leftMargin: 4
            height: parent.height
            spacing: 2
            RibbonButton {
                iconName: "hand_left"
                text: I18n.t("cmd.hand")
                shortcutHint: "Alt+3"
                checkable: true
                checked: true
                enabled: ribbon.hasDocument
            }
            Separator {}
            RibbonButton {
                iconName: "zoom_in"
                text: I18n.t("cmd.zoom")
                enabled: ribbon.hasDocument
                menu: zoomMenu
            }
            RibbonButton {
                iconName: "page_fit"
                text: I18n.t("cmd.pageFit")
                enabled: ribbon.hasDocument
                menu: fitMenu
            }
            RibbonButton {
                iconName: "arrow_rotate_clockwise"
                text: I18n.t("cmd.rotateView")
                enabled: ribbon.hasDocument
                menu: rotateMenu
            }
        }

        // View
        RowLayout {
            visible: ribbon.currentTab === 2
            anchors.left: parent.left
            anchors.leftMargin: 4
            height: parent.height
            spacing: 2
            RibbonButton {
                iconName: "book_open"
                text: I18n.t("cmd.readMode")
                shortcutHint: "Ctrl+H"
                enabled: ribbon.hasDocument
                checkable: true
                checked: AppState.readMode
                onClicked: AppState.readMode = !AppState.readMode
            }
            RibbonButton {
                iconName: "full_screen_maximize"
                text: I18n.t("cmd.fullScreen")
                shortcutHint: "F11"
                checkable: true
                checked: AppState.fullScreen
                onClicked: AppState.fullScreen = !AppState.fullScreen
            }
            RibbonButton {
                iconName: "color"
                text: I18n.t("cmd.visualMode")
                menu: colorsMenu
            }
            Separator {}
            RibbonButton {
                iconName: "arrow_rotate_clockwise"
                text: I18n.t("cmd.rotateView")
                enabled: ribbon.hasDocument
                menu: rotateMenu
            }
            Separator {}
            RibbonButton {
                iconName: "settings"
                text: I18n.t("cmd.viewSetting")
                menu: viewSettingMenu
            }
        }

        // Accessibility
        RowLayout {
            visible: ribbon.currentTab === 3
            anchors.left: parent.left
            anchors.leftMargin: 4
            height: parent.height
            spacing: 2
            RibbonButton {
                iconName: "accessibility"
                text: I18n.t("cmd.readingOption")
                onClicked: ribbon.preferencesRequested("accessibility")
            }
            RibbonButton {
                iconName: "tablet"
                text: I18n.t("cmd.touchMode")
                checkable: true
                checked: preferences.touchMode
                onClicked: preferences.touchMode = !preferences.touchMode
            }
        }

        // Help
        RowLayout {
            visible: ribbon.currentTab === 4
            anchors.left: parent.left
            anchors.leftMargin: 4
            height: parent.height
            spacing: 2
            RibbonButton {
                iconName: "info"
                text: I18n.t("cmd.about")
                onClicked: ribbon.aboutRequested()
            }
        }

        AtlasIconButton {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 4
            implicitWidth: 20
            implicitHeight: 20
            iconSize: 14
            iconName: ribbon.collapsed ? "pin" : "chevron_up"
            text: ribbon.collapsed ? I18n.t("ribbon.pin") : I18n.t("ribbon.collapse")
            shortcutHint: "Ctrl+F11"
            onClicked: { preferences.ribbonCollapsed = !preferences.ribbonCollapsed; ribbon.peek = false }
        }
    }

    // bottom line (Foxit: 4 px padding then a 1 px line)
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.chromeLine }

    // ---- menus
    Menu {
        id: zoomMenu
        MenuItem { text: I18n.t("zoom.in"); onTriggered: ribbon.viewport.zoomBy(1.25) }
        MenuItem { text: I18n.t("zoom.out"); onTriggered: ribbon.viewport.zoomBy(0.8) }
        MenuSeparator {}
        Repeater {
            model: [0.5, 0.75, 1, 1.25, 1.5, 2, 4]
            delegate: MenuItem {
                required property var modelData
                text: I18n.t("cmd.zoomTo", I18n.isolate(Math.round(modelData * 100) + "%"))
                onTriggered: ribbon.zoomTo(modelData)
            }
        }
    }
    Menu {
        id: fitMenu
        MenuItem {
            text: I18n.t("zoom.actualSize") + "\tCtrl+1"
            checkable: true
            checked: ribbon.viewport !== null && ribbon.viewport.fitMode === ReaderViewport.None && Math.abs(ribbon.viewport.zoom - 1) < 0.001
            onTriggered: ribbon.viewport.actualSize()
        }
        MenuItem {
            text: I18n.t("zoom.fitPage") + "\tCtrl+0"
            checkable: true
            checked: ribbon.viewport !== null && ribbon.viewport.fitMode === ReaderViewport.Page
            onTriggered: ribbon.viewport.fitMode = ReaderViewport.Page
        }
        MenuItem {
            text: I18n.t("zoom.fitWidth") + "\tCtrl+2"
            checkable: true
            checked: ribbon.viewport !== null && ribbon.viewport.fitMode === ReaderViewport.Width
            onTriggered: ribbon.viewport.fitMode = ReaderViewport.Width
        }
    }
    Menu {
        id: rotateMenu
        MenuItem { text: I18n.t("cmd.rotateClockwise") + "\tCtrl+Shift++"; onTriggered: ribbon.viewport.rotateView(1) }
        MenuItem { text: I18n.t("cmd.rotateCounterclockwise") + "\tCtrl+Shift+-"; onTriggered: ribbon.viewport.rotateView(-1) }
    }
    Menu {
        id: colorsMenu
        Repeater {
            model: 4
            delegate: MenuItem {
                required property int index
                text: I18n.t("view.colors." + index)
                checkable: true
                checked: preferences.readingColors === index
                onTriggered: preferences.readingColors = index
            }
        }
    }
    Menu {
        id: viewSettingMenu
        Menu {
            title: I18n.t("cmd.statusBar")
            MenuItem { text: I18n.t("cmd.statusShow"); checkable: true; checked: preferences.statusBarMode === 0; onTriggered: preferences.statusBarMode = 0 }
            MenuItem { text: I18n.t("cmd.statusAuto"); checkable: true; checked: preferences.statusBarMode === 1; onTriggered: preferences.statusBarMode = 1 }
            MenuItem { text: I18n.t("cmd.statusHide"); checkable: true; checked: preferences.statusBarMode === 2; onTriggered: preferences.statusBarMode = 2 }
        }
        MenuItem { text: I18n.t("nav.library"); onTriggered: readerController.showLibrary() }
        MenuItem {
            text: I18n.t("cmd.fullScreen") + "\tF11"
            checkable: true
            checked: AppState.fullScreen
            onTriggered: AppState.fullScreen = !AppState.fullScreen
        }
    }
}
