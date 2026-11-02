import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Atlas.Reader

// Composes the application window. Views live in their own files; tokens in
// Theme, strings in I18n (docs/DESIGN.md §11).
ApplicationWindow {
    id: root

    width: 1280
    height: 820
    minimumWidth: 760
    minimumHeight: 560
    visible: true
    color: Theme.canvas
    visibility: AppState.fullScreen ? Window.FullScreen : Window.AutomaticVisibility
    // Foxit-style custom title bar; --native-frame keeps the system caption.
    flags: nativeFrame ? Qt.Window : (Qt.Window | Qt.FramelessWindowHint)
    readonly property bool customFrame: !nativeFrame && !AppState.fullScreen
    readonly property var activeViewport: inReader && reader.currentTab && reader.currentTab.ready ? reader.currentTab.viewport : null
    title: readerController.activeIndex >= 0 && reader.currentTab
        ? reader.currentTab.title + " — " + I18n.t("app.name")
        : I18n.t("library.title") + " — " + I18n.t("app.name")

    font.family: I18n.uiFamily
    font.pixelSize: Math.round(14 * I18n.sizeScale)

    palette.window: Theme.canvas
    palette.windowText: Theme.ink
    palette.base: Theme.raised
    palette.alternateBase: Theme.panel
    palette.text: Theme.ink
    palette.button: Theme.raised
    palette.buttonText: Theme.ink
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    palette.placeholderText: Theme.muted
    palette.toolTipBase: Theme.raised
    palette.toolTipText: Theme.ink
    palette.mid: Theme.line
    palette.dark: Theme.controlBorder
    palette.light: Theme.panel

    LayoutMirroring.enabled: I18n.rtl
    LayoutMirroring.childrenInherit: true

    // Bundled fonts (assets/fonts, OFL): Urdu interface and Arabic/Urdu content.
    FontLoader { source: "assets/fonts/NotoNastaliqUrdu-wght.ttf" }
    FontLoader { source: "assets/fonts/NotoNaskhArabic-Regular.ttf" }
    FontLoader { source: "assets/fonts/NotoNaskhArabic-SemiBold.ttf" }

    Binding { target: I18n; property: "language"; value: preferences.language }
    Binding { target: Theme; property: "dark"; value: preferences.dark }
    Binding { target: Theme; property: "touch"; value: preferences.touchMode }
    Binding { target: Theme; property: "scriptExtra"; value: I18n.language === "ur" ? 10 : 0 }
    Binding { target: Theme; property: "colorful"; value: preferences.skin === "colorful" }
    Binding { target: Theme; property: "themeColor"; value: preferences.themeColor }

    readonly property bool inReader: readerController.activeIndex >= 0
    property string errorText: ""

    // ---- shortcuts (Foxit defaults, FOXIT_PARITY §4)
    Shortcut { sequence: "Ctrl+O"; onActivated: pdfDialog.open() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: folderDialog.open() }
    Shortcut { sequence: "Ctrl+K"; onActivated: preferencesDialog.openPage("general") }
    Shortcut { sequence: "Ctrl+F"; enabled: !root.inReader; onActivated: libraryView.searchField.forceActiveFocus() }
    // Reader text (N4.5, FOXIT_PARITY §4)
    Shortcut { sequence: "Ctrl+F"; enabled: root.activeViewport !== null; onActivated: reader.currentTab.openFind() }
    Shortcut { sequence: "Ctrl+Shift+F"; enabled: root.activeViewport !== null; onActivated: reader.currentTab.openSearch() }
    Shortcut { sequence: "F3"; enabled: root.activeViewport !== null && root.activeViewport.searchHits.length > 0; onActivated: root.activeViewport.findNext() }
    Shortcut { sequence: "Shift+F3"; enabled: root.activeViewport !== null && root.activeViewport.searchHits.length > 0; onActivated: root.activeViewport.findPrevious() }
    Shortcut { sequence: "Alt+3"; enabled: root.activeViewport !== null; onActivated: root.activeViewport.tool = ReaderViewport.HandTool }
    Shortcut { sequence: "Alt+6"; enabled: root.activeViewport !== null; onActivated: root.activeViewport.tool = ReaderViewport.SelectTool }
    Shortcut { sequence: "Alt+7"; enabled: root.activeViewport !== null; onActivated: root.activeViewport.tool = ReaderViewport.SnapshotTool }
    Shortcut { sequence: "Ctrl+Shift+A"; enabled: root.activeViewport !== null; onActivated: root.activeViewport.clearSelection() }
    Shortcut { sequence: "Ctrl+M"; enabled: root.activeViewport !== null; onActivated: readerDialogs.openZoomTo(root.activeViewport) }
    Shortcut { sequence: "Ctrl+Shift+H"; enabled: root.activeViewport !== null; onActivated: root.activeViewport.toggleAutoScroll() }
    Shortcut { sequences: ["Ctrl+W", "Ctrl+F4"]; enabled: root.inReader; onActivated: readerController.closeActive() }
    Shortcut { sequence: "Ctrl+Shift+W"; enabled: readerController.hasSessions; onActivated: readerController.closeAll() }
    Shortcut { sequence: "Ctrl+Q"; onActivated: root.close() }
    Shortcut { sequences: ["Ctrl+Tab", "Ctrl+Shift+Page Down"]; enabled: readerController.hasSessions; onActivated: reader.cycleTab(1) }
    Shortcut { sequences: ["Ctrl+Shift+Tab", "Ctrl+Shift+Page Up"]; enabled: readerController.hasSessions; onActivated: reader.cycleTab(-1) }
    Shortcut { sequence: "Ctrl+G"; enabled: root.inReader && !AppState.chromeHidden; onActivated: if (reader.currentTab) reader.currentTab.focusPageBox() }
    Shortcut { sequence: "Ctrl+H"; enabled: root.inReader; onActivated: AppState.readMode = !AppState.readMode }
    Shortcut { sequence: "Ctrl+F11"; onActivated: { preferences.ribbonCollapsed = !preferences.ribbonCollapsed; ribbon.peek = false } }
    Shortcut { sequence: "F11"; onActivated: AppState.fullScreen = !AppState.fullScreen }
    Shortcut { sequence: "F4"; enabled: root.inReader; onActivated: preferences.navPanel = preferences.navPanel === "" ? "bookmarks" : "" }
    Shortcut {
        sequence: "Esc"
        enabled: AppState.chromeHidden || backstage.visible || ribbon.peek
        onActivated: {
            if (backstage.visible) { backstage.visible = false; return }
            if (ribbon.peek) { ribbon.peek = false; return }
            AppState.readMode = false; AppState.fullScreen = false
        }
    }
    Shortcut { sequence: "F5"; enabled: !root.inReader; onActivated: libraryController.rescan() }

    FolderDialog {
        id: folderDialog
        title: I18n.t("dialog.chooseFolder")
        onAccepted: libraryController.addRoot(selectedFolder)
    }
    FileDialog {
        id: pdfDialog
        title: I18n.t("dialog.openPdf")
        nameFilters: [I18n.t("dialog.pdfFilter") + " (*.pdf)"]
        onAccepted: readerController.openLocalFile(selectedFile)
    }

    Connections {
        target: libraryController
        function onOperationError(message) {
            console.warn("Atlas operation failed:", message)
            root.errorText = I18n.t("error.libraryUnchanged")
            errorPopup.open()
        }
    }

    Popup {
        id: errorPopup
        x: root.LayoutMirroring.enabled ? Theme.xl : root.width - width - Theme.xl
        y: Theme.titleBarHeight + Theme.lg
        width: Math.min(440, root.width - 40)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: Theme.dangerSoft; radius: Theme.radiusControl; border.color: Theme.dangerBorder }
        contentItem: Label {
            text: root.errorText
            color: Theme.danger
            wrapMode: Text.WordWrap
            padding: Theme.md
        }
    }

    PreferencesDialog { id: preferencesDialog }
    ReaderDialogs { id: readerDialogs; z: 50 }

    // Visual QA (--show): open one surface after startup.
    Timer {
        interval: 1500
        running: qaShow.length > 0
        onTriggered: {
            if (qaShow === "backstage") { backstage.page = "open"; backstage.visible = true }
            else if (qaShow === "skins") { backstage.page = "skins"; backstage.visible = true }
            else if (qaShow.indexOf("preferences") === 0) preferencesDialog.openPage(qaShow.split(":")[1] || "general")
            else if (qaShow === "about") aboutDialog.open()
            else if (qaShow.indexOf("display:") === 0 && root.activeViewport) {
                const parts = qaShow.split(":")
                root.activeViewport.pageDisplay = parseInt(parts[1])
                root.activeViewport.rightToLeft = parts[2] === "rtl"
                ribbon.currentTab = 2
            }
            else if (qaShow.indexOf("find:") === 0 && reader.currentTab) { reader.currentTab.openFind(); reader.currentTab.findBarRun(qaShow.slice(5)) }
            else if (qaShow.indexOf("search:") === 0 && reader.currentTab) { reader.currentTab.openSearch(); reader.currentTab.searchRun(qaShow.slice(7)) }
            else if (qaShow === "capability" && reader.currentTab) reader.currentTab.showCapability()
            else if (qaShow === "bookmarks") preferences.navPanel = "bookmarks"
            else if (qaShow === "wordcount" && root.activeViewport) readerDialogs.openWordCount(root.activeViewport)
            else if (qaShow === "zoomto" && root.activeViewport) readerDialogs.openZoomTo(root.activeViewport)
            else if (qaShow === "link") readerDialogs.confirmLink("https://example.org/atlas")
            else if (qaShow === "selectall" && root.activeViewport) { root.activeViewport.goToPage(4); root.activeViewport.selectAllOnPage() }
            else if (qaShow.indexOf("ribbon-") === 0) ribbon.currentTab = ({ home: 1, view: 2, accessibility: 3, help: 4 })[qaShow.slice(7)] || 1
        }
    }
    AboutDialog { id: aboutDialog }

    // Window border and resize edges for the custom frame.
    Rectangle {
        anchors.fill: parent
        z: 100
        visible: root.customFrame && root.visibility !== Window.Maximized
        color: "transparent"
        border.width: 1
        border.color: Theme.chromeLine
    }
    Repeater {
        model: root.customFrame && root.visibility !== Window.Maximized ? [
            { edges: Qt.LeftEdge, x: 0, y: 6, w: 5, h: -12, cursor: Qt.SizeHorCursor },
            { edges: Qt.RightEdge, x: -5, y: 6, w: 5, h: -12, cursor: Qt.SizeHorCursor },
            { edges: Qt.TopEdge, x: 6, y: 0, w: -12, h: 4, cursor: Qt.SizeVerCursor },
            { edges: Qt.BottomEdge, x: 6, y: -5, w: -12, h: 5, cursor: Qt.SizeVerCursor },
            { edges: Qt.LeftEdge | Qt.TopEdge, x: 0, y: 0, w: 6, h: 6, cursor: Qt.SizeFDiagCursor },
            { edges: Qt.RightEdge | Qt.BottomEdge, x: -6, y: -6, w: 6, h: 6, cursor: Qt.SizeFDiagCursor },
            { edges: Qt.RightEdge | Qt.TopEdge, x: -6, y: 0, w: 6, h: 6, cursor: Qt.SizeBDiagCursor },
            { edges: Qt.LeftEdge | Qt.BottomEdge, x: 0, y: -6, w: 6, h: 6, cursor: Qt.SizeBDiagCursor }
        ] : []
        delegate: MouseArea {
            required property var modelData
            z: 101
            x: modelData.x >= 0 ? modelData.x : root.width + modelData.x
            y: modelData.y >= 0 ? modelData.y : root.height + modelData.y
            width: modelData.w > 0 ? modelData.w : root.width + modelData.w
            height: modelData.h > 0 ? modelData.h : root.height + modelData.h
            cursorShape: modelData.cursor
            acceptedButtons: Qt.LeftButton
            onPressed: root.startSystemResize(modelData.edges)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            visible: !(AppState.chromeHidden && root.inReader)
            customFrame: root.customFrame
            placeTitle: root.inReader && reader.currentTab ? reader.currentTab.title : ""
            onOpenPdfRequested: pdfDialog.open()
        }

        RibbonBar {
            id: ribbon
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            visible: !(AppState.chromeHidden && root.inReader)
            viewport: root.activeViewport
            backstageOpen: backstage.visible
            onFileRequested: { backstage.page = "open"; backstage.visible = !backstage.visible; if (backstage.visible) backstage.forceActiveFocus() }
            onAboutRequested: aboutDialog.open()
            onPreferencesRequested: (page) => preferencesDialog.openPage(page)
            onFindRequested: if (reader.currentTab) reader.currentTab.openFind()
            onSearchRequested: if (reader.currentTab) reader.currentTab.openSearch()
            onZoomToRequested: if (root.activeViewport) readerDialogs.openZoomTo(root.activeViewport)
            onWordCountRequested: if (root.activeViewport) readerDialogs.openWordCount(root.activeViewport)
            onCurrentTabChanged: backstage.visible = false
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle { // 7 px gap between ribbon and document tabs (Foxit)
                    Layout.fillWidth: true
                    Layout.preferredHeight: 7
                    visible: !AppState.chromeHidden
                    color: Theme.sunken
                }
                DocumentTabBar {
                    Layout.fillWidth: true
                    visible: !AppState.chromeHidden
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    LibraryView {
                        id: libraryView
                        anchors.fill: parent
                        visible: !root.inReader
                        onAddFolderRequested: folderDialog.open()
                    }
                    ReaderView {
                        id: reader
                        anchors.fill: parent
                        visible: root.inReader
                        onLinkConfirmRequested: (url) => readerDialogs.confirmLink(url)
                        onToastRequested: (text) => readerDialogs.showToast(text)
                    }
                }
            }

            Backstage {
                id: backstage
                anchors.fill: parent
                visible: false
                z: 10
                onCloseRequested: visible = false
                onOpenFileRequested: { visible = false; pdfDialog.open() }
                onPreferencesRequested: { visible = false; preferencesDialog.openPage("general") }
                onExitRequested: root.close()
            }

            // Clicking outside a temporarily shown (collapsed) ribbon hides it.
            MouseArea {
                anchors.fill: parent
                visible: ribbon.peek
                z: 9
                onPressed: (mouse) => { ribbon.peek = false; mouse.accepted = false }
            }
        }
    }
}
