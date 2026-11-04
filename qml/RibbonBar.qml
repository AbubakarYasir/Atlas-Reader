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
    property int currentTab: 1           // 0 File, 1 Home, 2 Edit, 3 View, 4 Accessibility, 5 Help
    property bool backstageOpen: false
    property bool peek: false            // collapsed ribbon shown temporarily
    readonly property bool hasDocument: viewport !== null
    readonly property bool collapsed: preferences.ribbonCollapsed
    readonly property bool showCommands: !collapsed || peek

    signal fileRequested()
    signal aboutRequested()
    signal preferencesRequested(string page)
    signal findRequested()
    signal bookmarkRequested()
    signal autoBookmarksRequested()
    signal searchRequested()
    signal zoomToRequested()
    signal wordCountRequested()

    function setTool(tool) { if (viewport) viewport.tool = tool }

    implicitHeight: Theme.ribbonTabHeight + (showCommands && !backstageOpen ? Theme.ribbonHeight + 5 : 1)
    color: Theme.chrome

    readonly property var tabs: [
        { key: "ribbon.file" }, { key: "ribbon.home" }, { key: "ribbon.edit" }, { key: "ribbon.view" },
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
                checked: !ribbon.hasDocument || ribbon.viewport.tool === ReaderViewport.HandTool
                enabled: ribbon.hasDocument
                onClicked: ribbon.setTool(ReaderViewport.HandTool)
            }
            RibbonButton {
                iconName: "cursor"
                text: I18n.t("cmd.selectText")
                shortcutHint: "Alt+6"
                checkable: true
                checked: ribbon.hasDocument && ribbon.viewport.tool === ReaderViewport.SelectTool
                enabled: ribbon.hasDocument
                onClicked: ribbon.setTool(ReaderViewport.SelectTool)
            }
            RibbonButton {
                iconName: "screenshot"
                text: I18n.t("cmd.snapshot")
                shortcutHint: "Alt+7"
                checkable: true
                checked: ribbon.hasDocument && ribbon.viewport.tool === ReaderViewport.SnapshotTool
                enabled: ribbon.hasDocument
                onClicked: ribbon.setTool(ReaderViewport.SnapshotTool)
            }
            RibbonButton {
                iconName: "clipboard"
                text: I18n.t("cmd.clipboard")
                enabled: ribbon.hasDocument
                menu: clipboardMenu
            }
            RibbonButton {
                iconName: "bookmark"
                text: I18n.t("cmd.bookmark")
                shortcutHint: "Ctrl+B"
                enabled: ribbon.hasDocument
                onClicked: ribbon.bookmarkRequested()
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
            Separator {}
            RibbonButton {
                iconName: "search"
                text: I18n.t("find.title")
                shortcutHint: "Ctrl+F"
                enabled: ribbon.hasDocument
                onClicked: ribbon.findRequested()
            }
            RibbonButton {
                iconName: "search_square"
                text: I18n.t("panel.search")
                shortcutHint: "Ctrl+Shift+F"
                enabled: ribbon.hasDocument
                onClicked: ribbon.searchRequested()
            }
        }

        // Edit (Foxit: Hand, Select | Content | Add | Links: …, Bookmark, Auto Create
        // Bookmarks | Insert). Only the commands Atlas can do today are shown.
        RowLayout {
            visible: ribbon.currentTab === 2
            anchors.left: parent.left
            anchors.leftMargin: 4
            height: parent.height
            spacing: 2
            RibbonButton {
                iconName: "hand_left"
                text: I18n.t("cmd.hand")
                shortcutHint: "Alt+3"
                checkable: true
                checked: !ribbon.hasDocument || ribbon.viewport.tool === ReaderViewport.HandTool
                enabled: ribbon.hasDocument
                onClicked: ribbon.setTool(ReaderViewport.HandTool)
            }
            RibbonButton {
                iconName: "cursor"
                text: I18n.t("cmd.selectText")
                shortcutHint: "Alt+6"
                checkable: true
                checked: ribbon.hasDocument && ribbon.viewport.tool === ReaderViewport.SelectTool
                enabled: ribbon.hasDocument
                onClicked: ribbon.setTool(ReaderViewport.SelectTool)
            }
            Separator {}
            RibbonButton {
                iconName: "bookmark"
                text: I18n.t("cmd.bookmark")
                shortcutHint: "Ctrl+B"
                enabled: ribbon.hasDocument
                onClicked: ribbon.bookmarkRequested()
            }
            RibbonButton {
                iconName: "text_bullet_list_tree"
                text: I18n.t("cmd.autoBookmarks")
                enabled: ribbon.hasDocument
                onClicked: ribbon.autoBookmarksRequested()
            }
        }

        // View
        RowLayout {
            visible: ribbon.currentTab === 3
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
                iconName: "arrow_bidirectional_left_right"
                text: I18n.t("cmd.reverseView")
                enabled: ribbon.hasDocument
                checkable: true
                checked: ribbon.hasDocument && ribbon.viewport.rightToLeft
                onClicked: ribbon.viewport.rightToLeft = !ribbon.viewport.rightToLeft
            }
            RibbonButton {
                iconName: "color"
                text: I18n.t("cmd.visualMode")
                menu: colorsMenu
            }
            Separator {}
            // Page Display (Foxit: small buttons, two rows)
            Grid {
                Layout.alignment: Qt.AlignVCenter
                rows: 2
                flow: Grid.TopToBottom
                spacing: 4
                Repeater {
                    model: [
                        { icon: "document_one_page", key: "cmd.singlePage", mode: 0 },
                        { icon: "document_page_break", key: "cmd.continuous", mode: 1 },
                        { icon: "book", key: "cmd.facing", mode: 2 },
                        { icon: "document_landscape_split", key: "cmd.continuousFacing", mode: 3 }
                    ]
                    delegate: RibbonSmallButton {
                        required property var modelData
                        iconName: modelData.icon
                        text: I18n.t(modelData.key)
                        enabled: ribbon.hasDocument
                        checkable: true
                        checked: ribbon.hasDocument && ribbon.viewport.pageDisplay === modelData.mode
                        onClicked: ribbon.viewport.pageDisplay = modelData.mode
                    }
                }
                RibbonSmallButton {
                    iconName: "book_open"
                    text: I18n.t("cmd.coverPage")
                    enabled: ribbon.hasDocument && ribbon.viewport.pageDisplay >= 2
                    checkable: true
                    checked: ribbon.hasDocument && ribbon.viewport.coverPage
                    onClicked: ribbon.viewport.coverPage = !ribbon.viewport.coverPage
                }
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
                iconName: "arrow_autofit_down"
                text: I18n.t("cmd.autoScroll")
                shortcutHint: "Ctrl+Shift+H"
                enabled: ribbon.hasDocument
                checkable: true
                checked: ribbon.hasDocument && ribbon.viewport.autoScrolling
                onClicked: ribbon.viewport.toggleAutoScroll()
            }
            RibbonButton {
                iconName: "text_word_count"
                text: I18n.t("wordCount.title")
                enabled: ribbon.hasDocument
                onClicked: ribbon.wordCountRequested()
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
            visible: ribbon.currentTab === 4
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
            visible: ribbon.currentTab === 5
            anchors.left: parent.left
            anchors.leftMargin: 4
            height: parent.height
            spacing: 2
            RibbonButton {
                iconName: "info"
                text: I18n.t("cmd.about")
                onClicked: ribbon.aboutRequested()
            }
            RibbonButton {
                iconName: "open"
                text: I18n.t("cmd.dereksol")
                onClicked: readerController.openExternalLink("https://dereksol.com")
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
        id: clipboardMenu
        MenuItem {
            text: I18n.t("action.copy") + "\tCtrl+C"
            enabled: ribbon.hasDocument && ribbon.viewport.selectedText.length > 0
            onTriggered: ribbon.viewport.copySelection()
        }
        MenuItem {
            text: I18n.t("action.selectAll") + "\tCtrl+A"
            enabled: ribbon.hasDocument && ribbon.viewport.hasText
            onTriggered: ribbon.viewport.selectAllOnPage()
        }
        MenuItem {
            text: I18n.t("action.deselectAll")
            enabled: ribbon.hasDocument && ribbon.viewport.selectedText.length > 0
            onTriggered: ribbon.viewport.clearSelection()
        }
    }
    Menu {
        id: zoomMenu
        MenuItem { text: I18n.t("zoom.in") + "\tCtrl+="; onTriggered: ribbon.viewport.zoomBy(1.25) }
        MenuItem { text: I18n.t("zoom.out") + "\tCtrl+-"; onTriggered: ribbon.viewport.zoomBy(0.8) }
        MenuItem {
            text: I18n.t("cmd.marqueeZoom")
            checkable: true
            checked: ribbon.hasDocument && ribbon.viewport.tool === ReaderViewport.MarqueeTool
            onTriggered: ribbon.setTool(ReaderViewport.MarqueeTool)
        }
        MenuItem { text: I18n.t("zoom.zoomTo") + "\tCtrl+M"; onTriggered: ribbon.zoomToRequested() }
        MenuSeparator {}
        Repeater {
            // Foxit's zoom list, 1 % to 6400 %
            model: [0.0833, 0.125, 0.25, 0.3333, 0.5, 0.6667, 0.75, 1, 1.25, 1.5, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64]
            delegate: MenuItem {
                required property var modelData
                text: I18n.t("cmd.zoomTo", I18n.isolate((Math.round(modelData * 10000) / 100) + "%"))
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
        MenuItem {
            text: I18n.t("zoom.fitVisible") + "\tCtrl+3"
            onTriggered: ribbon.viewport.fitVisible()
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
        Menu {
            title: I18n.t("panel.navigation")
            MenuItem { text: I18n.t("panel.show") + "\tF4"; enabled: preferences.navPanel === ""; onTriggered: preferences.navPanel = "bookmarks" }
            MenuSeparator {}
            MenuItem { text: I18n.t("panel.bookmarks"); checkable: true; checked: preferences.navPanel === "bookmarks"; onTriggered: preferences.navPanel = checked ? "bookmarks" : "" }
            MenuItem { text: I18n.t("panel.pages"); checkable: true; checked: preferences.navPanel === "pages"; onTriggered: preferences.navPanel = checked ? "pages" : "" }
            MenuSeparator {}
            MenuItem {
                text: I18n.t("panel.reset")
                onTriggered: { preferences.navPanel = "bookmarks"; preferences.navPanelWidth = 240; preferences.rightPanel = "" }
            }
            MenuItem { text: I18n.t("panel.hide") + "\tF4"; enabled: preferences.navPanel !== ""; onTriggered: preferences.navPanel = "" }
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
