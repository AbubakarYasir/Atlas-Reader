import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Atlas.Reader

// One open document: page viewport and Foxit-style status bar, plus the
// status view used while loading, for passwords and for errors (DESIGN.md §3).
Rectangle {
    id: tab

    required property int index
    required property string title
    required property string sourcePath
    required property string status
    required property int pageCount
    required property string detail
    required property string documentTitle
    required property var sessionId
    required property var capability     // N5.1 capability check (ReaderController)

    readonly property bool ready: status === "ready"
    readonly property alias viewport: view
    // The book prints its own page numbers (roman front matter, etc.).
    readonly property bool hasLabels: {
        const labels = view.pageLabels
        for (let i = 0; i < labels.length; ++i) if (labels[i] !== String(i + 1)) return true
        return false
    }
    function pageLabel(page) {
        const labels = view.pageLabels
        return hasLabels && page >= 1 && page <= labels.length ? labels[page - 1] : String(page)
    }

    color: ready ? Theme.readerBackdrop : Theme.canvas

    signal linkConfirmRequested(string url)
    signal toastRequested(string text)

    function focusPageBox() { pageBox.forceActiveFocus() }
    function showCapability() { if (capabilityChip.visible) capabilityPopup.open() }
    // Coming back to a tab re-checks the file (it may have changed outside Atlas).
    onVisibleChanged: if (visible && ready && capability && capability.checked) readerController.recheckCapabilities(index)
    function openFind() {
        if (preferences.rightPanel === "search") { searchPanel.focusField(view.selectedText); return }
        findBar.open(view.selectedText.length < 80 ? view.selectedText : "")
    }
    function openSearch() {
        findBar.visible = false
        preferences.rightPanel = "search"
        searchPanel.focusField(view.selectedText.length < 80 ? view.selectedText : (view.searchQuery || ""))
    }
    // Visual QA (--show find:/search:)
    function findBarRun(text) { findBar.open(text); findBar.run() }
    function searchRun(text) { searchPanel.focusField(text); searchPanel.run() }
    function closeSearch() { preferences.rightPanel = ""; view.forceActiveFocus() }

    // ---- per-book reading position (Preferences → Documents / History)
    property bool restored: false
    function restorePosition() {
        if (restored || view.pageCount <= 0) return
        restored = true
        const saved = preferences.positionFor(tab.sourcePath)
        if (saved.page !== undefined) {
            view.pageDisplay = saved.display
            if (saved.fit === 1) view.fitMode = ReaderViewport.Width
            else if (saved.fit === 2) view.fitMode = ReaderViewport.Page
            else view.zoom = saved.zoom
            view.goToPage(saved.page)
        } else {
            view.pageDisplay = preferences.defaultPageDisplay
            if (preferences.defaultZoom === 1) view.fitMode = ReaderViewport.Page
            else if (preferences.defaultZoom === 2) view.actualSize()
            else view.fitMode = ReaderViewport.Width
        }
    }
    function savePosition() {
        if (!restored || view.pageCount <= 0) return
        const fit = view.fitMode === ReaderViewport.Width ? 1 : view.fitMode === ReaderViewport.Page ? 2 : 0
        preferences.savePosition(tab.sourcePath, view.currentPage, view.zoom, fit, view.pageDisplay)
    }
    Timer { id: saveTimer; interval: 1200; onTriggered: tab.savePosition() }
    Connections {
        target: view
        function onLayoutChanged() { tab.restorePosition() }
        function onCurrentPageChanged() { if (tab.restored) saveTimer.restart() }
        function onZoomChanged() { if (tab.restored) saveTimer.restart() }
        function onPageDisplayChanged() { if (tab.restored) saveTimer.restart() }
        function onExternalLinkRequested(url) { tab.linkConfirmRequested(url) }
        function onSnapshotCopied() { tab.toastRequested(I18n.t("toast.snapshotCopied")) }
        function onContextMenuRequested(x, y) {
            pageMenu.link = view.hoveredLink
            pageMenu.popup(view, x, y)
        }
    }
    Component.onDestruction: savePosition()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: tab.ready

        // ---- navigation rail and panel (Foxit navigation pane, F4)
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: Theme.railWidth
                Layout.fillHeight: true
                visible: !AppState.chromeHidden
                color: Theme.rail
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.chromeLine }
                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 18
                    spacing: 8
                    Repeater {
                        model: [
                            { panel: "bookmarks", icon: "bookmark", key: "panel.bookmarks" },
                            { panel: "pages", icon: "document_copy", key: "panel.pages" }
                        ]
                        delegate: AtlasIconButton {
                            required property var modelData
                            implicitWidth: Theme.railWidth - 6
                            implicitHeight: Theme.railWidth - 6
                            iconSize: 20
                            cornerRadius: 2
                            iconName: modelData.icon
                            text: I18n.t(modelData.key)
                            checkable: true
                            checked: preferences.navPanel === modelData.panel
                            onClicked: preferences.navPanel = preferences.navPanel === modelData.panel ? "" : modelData.panel
                        }
                    }
                }
            }

            PagesPanel {
                id: pagesPanel
                Layout.preferredWidth: preferences.navPanelWidth
                Layout.fillHeight: true
                visible: preferences.navPanel === "pages" && !AppState.chromeHidden
                view: view
                sessionId: tab.sessionId
            }
            BookmarksPanel {
                id: bookmarksPanel
                Layout.preferredWidth: preferences.navPanelWidth
                Layout.fillHeight: true
                visible: preferences.navPanel === "bookmarks" && !AppState.chromeHidden
                view: view
            }
            Rectangle { // splitter: drag to resize, double-click to hide (Foxit)
                Layout.preferredWidth: 6
                Layout.fillHeight: true
                visible: pagesPanel.visible || bookmarksPanel.visible
                color: splitterHover.hovered || splitterDrag.active ? Theme.hover : Theme.pressed
                Accessible.role: Accessible.Separator
                Accessible.name: I18n.t("panel.resize")
                HoverHandler { id: splitterHover; cursorShape: Qt.SizeHorCursor }
                DragHandler {
                    id: splitterDrag
                    target: null
                    xAxis.enabled: true
                    yAxis.enabled: false
                    property int startWidth: 0
                    onActiveChanged: if (active) startWidth = preferences.navPanelWidth
                    onTranslationChanged: preferences.navPanelWidth = startWidth + (tab.LayoutMirroring.enabled ? -translation.x : translation.x)
                }
                TapHandler { onDoubleTapped: preferences.navPanel = "" }
            }


            // ---- document pane
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ReaderViewport {
                    id: view
                    anchors.fill: parent
                    controller: readerController
                    sessionId: tab.sessionId
                    active: tab.visible && tab.ready
                    readingColors: preferences.readingColors
                    pageColor: Theme.readingPaper[preferences.readingColors]
                    pageShadowColor: Theme.pageShadow
                    focus: tab.visible
                    // Screen readers hear the page position, then the page's text (C04).
                    Accessible.role: Accessible.Document
                    Accessible.name: I18n.t("reader.pagesA11y", view.currentPage, view.pageCount)
                    Accessible.description: view.textReady && !view.hasText ? I18n.t("find.noText") : view.pageText
                }

                ScrollBar {
                    id: vbar
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    orientation: Qt.Vertical
                    policy: view.contentHeight > view.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                    size: view.contentHeight > 0 ? view.height / view.contentHeight : 1
                    position: view.contentHeight > 0 ? view.contentY / view.contentHeight : 0
                    onPositionChanged: if (pressed) view.contentY = position * view.contentHeight
                }
                ScrollBar {
                    anchors.left: parent.left
                    anchors.right: vbar.left
                    anchors.bottom: parent.bottom
                    orientation: Qt.Horizontal
                    policy: view.contentWidth > view.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                    size: view.contentWidth > 0 ? view.width / view.contentWidth : 1
                    position: view.contentWidth > 0 ? view.contentX / view.contentWidth : 0
                    onPositionChanged: if (pressed) view.contentX = position * view.contentWidth
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    running: view.loading
                    visible: running
                }
                Label {
                    anchors.centerIn: parent
                    width: Math.min(520, parent.width - 48)
                    visible: view.errorKey.length > 0
                    text: I18n.t("reader.renderError")
                    color: Theme.ink
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }

                // ---- banner: the document has no text layer (scanned pages)
                Rectangle {
                    id: noTextBanner
                    property bool dismissed: false
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: vbar.left
                    height: visible ? 32 : 0
                    visible: view.textReady && !view.hasText && !dismissed && !AppState.chromeHidden
                    color: Theme.warningSoft
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
                    Accessible.role: Accessible.AlertMessage
                    Accessible.name: I18n.t("find.noText")
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.md
                        anchors.rightMargin: Theme.xs
                        Label {
                            Layout.fillWidth: true
                            text: I18n.t("find.noText")
                            color: Theme.ink
                            elide: Text.ElideRight
                            font.pixelSize: Math.round(12 * I18n.sizeScale)
                        }
                        AtlasIconButton {
                            implicitWidth: 24; implicitHeight: 24; iconSize: 12; cornerRadius: 2
                            iconName: "dismiss"
                            text: I18n.t("action.close")
                            onClicked: noTextBanner.dismissed = true
                        }
                    }
                }

                FindBar {
                    id: findBar
                    view: view
                    anchors.top: noTextBanner.bottom
                    anchors.topMargin: Theme.sm
                    anchors.right: vbar.left
                    anchors.rightMargin: Theme.sm
                    onAdvancedRequested: tab.openSearch()
                }

                // AutoScroll indicator
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: Theme.sm
                    visible: view.autoScrolling
                    width: autoLabel.implicitWidth + Theme.lg
                    height: 26
                    radius: 13
                    color: Theme.inkStrong
                    opacity: 0.85
                    Label {
                        id: autoLabel
                        anchors.centerIn: parent
                        text: I18n.t("autoScroll.active")
                        color: Theme.raised
                        font.pixelSize: 12
                    }
                }

                // ---- read mode / full screen mini bar
                Rectangle {
                    id: miniBar
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: Theme.lg
                    visible: AppState.chromeHidden
                    opacity: miniHover.hovered || miniTimer.running || miniRow.activeFocus ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.normal } }
                    width: miniRow.implicitWidth + Theme.lg
                    height: Theme.commandBarHeight
                    radius: Theme.radiusSurface
                    color: Theme.panel
                    border.color: Theme.line
                    HoverHandler { id: miniHover }
                    Timer { id: miniTimer; interval: 2200 }
                    Connections {
                        target: view
                        function onCurrentPageChanged() { if (AppState.chromeHidden) miniTimer.restart() }
                    }
                    Connections {
                        target: AppState
                        function onChromeHiddenChanged() { if (AppState.chromeHidden) miniTimer.restart() }
                    }
                    RowLayout {
                        id: miniRow
                        anchors.centerIn: parent
                        spacing: 2
                        AtlasIconButton { iconName: "chevron_left"; mirrorIcon: true; text: I18n.t("nav.previousPage"); onClicked: view.previousPage() }
                        Label {
                            text: I18n.t("status.pageOf", view.currentPage, view.pageCount)
                            color: Theme.ink
                            font.pixelSize: 13
                            Layout.leftMargin: Theme.xs
                            Layout.rightMargin: Theme.xs
                        }
                        AtlasIconButton { iconName: "chevron_right"; mirrorIcon: true; text: I18n.t("nav.nextPage"); onClicked: view.nextPage() }
                        AtlasIconButton { iconName: "zoom_out"; text: I18n.t("zoom.out"); onClicked: view.zoomBy(0.8) }
                        AtlasIconButton { iconName: "zoom_in"; text: I18n.t("zoom.in"); onClicked: view.zoomBy(1.25) }
                        AtlasButton {
                            kind: "quiet"
                            iconName: "full_screen_minimize"
                            text: AppState.fullScreen ? I18n.t("view.exitFullScreen") : I18n.t("view.exitReadMode")
                            Accessible.description: "Esc"
                            onClicked: { AppState.readMode = false; AppState.fullScreen = false }
                        }
                    }
                }
            }

            SearchPanel {
                id: searchPanel
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                visible: preferences.rightPanel === "search" && !AppState.chromeHidden
                view: view
                onCloseRequested: tab.closeSearch()
            }
        }

        // ---- status bar (Foxit layout: navigation leading, zoom trailing)
        Item {
            id: statusArea
            Layout.fillWidth: true
            readonly property bool autoHide: preferences.statusBarMode === 1
            readonly property bool shown: !AppState.chromeHidden && preferences.statusBarMode !== 2
                && (!autoHide || statusHover.hovered || pageBox.activeFocus)
            Layout.preferredHeight: AppState.chromeHidden || preferences.statusBarMode === 2 ? 0
                : autoHide ? (shown ? Theme.statusBarHeight : 6) : Theme.statusBarHeight
            visible: Layout.preferredHeight > 0
            HoverHandler { id: statusHover }

        Rectangle {
            anchors.fill: parent
            visible: statusArea.shown
            color: Theme.chrome
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.chromeLine }

            RowLayout {
                anchors.fill: parent
                anchors.topMargin: 1
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 0

                component StatusButton: AtlasIconButton {
                    implicitWidth: 26
                    implicitHeight: 26
                    iconSize: 16
                    cornerRadius: 2
                    mirrorIcon: true
                }

                StatusButton { iconName: "arrow_previous"; text: I18n.t("nav.firstPage"); shortcutHint: "Ctrl+Home"; enabled: view.currentPage > 1; onClicked: view.firstPage() }
                StatusButton { iconName: "chevron_left"; text: I18n.t("nav.previousPage"); shortcutHint: "Ctrl+Page Up"; enabled: view.currentPage > 1; onClicked: view.previousPage() }
                Rectangle { // page box: "n / total" in one field, as in Foxit
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                    implicitWidth: pageRow.implicitWidth + 12
                    implicitHeight: Math.min(24 + Theme.scriptExtra, statusArea.height - 6)
                    radius: 2
                    color: Theme.raised
                    border.width: pageBox.activeFocus ? 2 : 1
                    border.color: pageBox.activeFocus ? Theme.focusRing : Theme.controlBorder
                    Row {
                        id: pageRow
                        anchors.centerIn: parent
                        LayoutMirroring.enabled: false   // "12 / 340" reads left to right in every language
                        spacing: 4
                        TextInput {
                            id: pageBox
                            width: Math.max(18, contentWidth)
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                            color: Theme.ink
                            selectionColor: Theme.accent
                            selectedTextColor: Theme.accentText
                            font.pixelSize: 12
                            selectByMouse: true
                            Accessible.role: Accessible.EditableText
                            Accessible.name: I18n.t("nav.goToPage")
                            Accessible.description: "Ctrl+G"
                            Binding on text {
                                when: !pageBox.activeFocus
                                value: tab.pageLabel(view.currentPage)
                            }
                            // Page labels first (Foxit: "iv", "A-3"), then plain page numbers.
                            onAccepted: {
                                const typed = text.trim()
                                if (!view.goToLabel(typed)) {
                                    const n = parseInt(typed)
                                    if (n >= 1 && n <= view.pageCount) view.goToPage(n)
                                }
                                view.forceActiveFocus()
                            }
                            onActiveFocusChanged: if (activeFocus) selectAll()
                            Keys.onEscapePressed: view.forceActiveFocus()
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: tab.hasLabels ? "(" + view.currentPage + " / " + view.pageCount + ")" : "/ " + view.pageCount
                            color: Theme.muted
                            font.pixelSize: 12
                        }
                    }
                }
                StatusButton { iconName: "chevron_right"; text: I18n.t("nav.nextPage"); shortcutHint: "Ctrl+Page Down"; enabled: view.currentPage < view.pageCount; onClicked: view.nextPage() }
                StatusButton { iconName: "arrow_next"; text: I18n.t("nav.lastPage"); shortcutHint: "End"; enabled: view.currentPage < view.pageCount; onClicked: view.lastPage() }
                Rectangle { width: 1; Layout.preferredHeight: 16; color: Theme.chromeLine; Layout.leftMargin: 4; Layout.rightMargin: 4 }
                // Previous/next view are temporal (Alt+Left/Right), so they never mirror.
                StatusButton { iconName: "arrow_left"; mirrorIcon: false; text: I18n.t("nav.previousView"); shortcutHint: "Alt+Left"; enabled: view.canGoBack; onClicked: view.goBack() }
                StatusButton { iconName: "arrow_right"; mirrorIcon: false; text: I18n.t("nav.nextView"); shortcutHint: "Alt+Right"; enabled: view.canGoForward; onClicked: view.goForward() }

                Item { Layout.fillWidth: true }

                // N5.1: shown only when Atlas may not change this PDF; explains why on click.
                AbstractButton {
                    id: capabilityChip
                    readonly property string state: tab.capability && tab.capability.checked ? tab.capability.state : ""
                    visible: state.length > 0 && state !== "writable"
                    Layout.rightMargin: Theme.sm
                    implicitHeight: 22
                    implicitWidth: chipRow.implicitWidth + 14
                    hoverEnabled: true
                    Accessible.role: Accessible.Button
                    Accessible.name: I18n.t("capability.title") + ": " + chipLabel.text
                    onClicked: capabilityPopup.open()
                    background: Rectangle {
                        radius: 11
                        color: capabilityChip.hovered ? Theme.hover : Theme.warningSoft
                        border.width: capabilityChip.visualFocus ? 2 : 1
                        border.color: capabilityChip.visualFocus ? Theme.focusRing : Theme.line
                    }
                    contentItem: Row {
                        id: chipRow
                        anchors.centerIn: parent
                        spacing: 4
                        IconImage {
                            anchors.verticalCenter: parent.verticalCenter
                            source: Theme.icon("lock_closed")
                            sourceSize.width: 12
                            sourceSize.height: 12
                            color: Theme.ink
                        }
                        Text {
                            id: chipLabel
                            anchors.verticalCenter: parent.verticalCenter
                            text: capabilityChip.state.length > 0 ? I18n.t("capability.state." + capabilityChip.state) : ""
                            color: Theme.ink
                            font.family: I18n.uiFamily
                            font.pixelSize: Math.round(11 * I18n.sizeScale)
                        }
                    }
                    Popup {
                        id: capabilityPopup
                        y: -implicitHeight - 6
                        x: capabilityChip.mirrored ? 0 : capabilityChip.width - width
                        width: 340
                        padding: Theme.md
                        focus: true
                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                        background: Rectangle { color: Theme.raised; radius: Theme.radiusSurface; border.color: Theme.chromeLine }
                        contentItem: ColumnLayout {
                            spacing: Theme.sm
                            Accessible.role: Accessible.Dialog
                            Accessible.name: I18n.t("capability.title")
                            Label {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: I18n.t("capability.title")
                                color: Theme.ink
                                font.family: I18n.uiFamily
                                font.weight: Font.DemiBold
                            }
                            Repeater {
                                model: tab.capability && tab.capability.reasons ? tab.capability.reasons : []
                                delegate: Label {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignLeft
                                    text: "• " + I18n.t("capability.reason." + modelData)
                                    color: Theme.ink
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                visible: tab.capability && tab.capability.canKeepLocalResearch
                                text: I18n.t("capability.local")
                                color: Theme.muted
                                wrapMode: Text.WordWrap
                                font.pixelSize: Math.round(12 * I18n.sizeScale)
                            }
                            AtlasButton {
                                Layout.alignment: Qt.AlignRight
                                iconName: "arrow_sync"
                                text: I18n.t("capability.recheck")
                                onClicked: { readerController.recheckCapabilities(tab.index); capabilityPopup.close() }
                            }
                        }
                    }
                }

                Repeater { // page display (Foxit status bar)
                    model: [
                        { icon: "document_one_page", key: "cmd.singlePage", mode: 0 },
                        { icon: "document_page_break", key: "cmd.continuous", mode: 1 },
                        { icon: "book", key: "cmd.facing", mode: 2 },
                        { icon: "document_landscape_split", key: "cmd.continuousFacing", mode: 3 }
                    ]
                    delegate: StatusButton {
                        required property var modelData
                        mirrorIcon: false
                        iconName: modelData.icon
                        text: I18n.t(modelData.key)
                        checkable: true
                        checked: view.pageDisplay === modelData.mode
                        onClicked: view.pageDisplay = modelData.mode
                    }
                }
                Rectangle { width: 1; Layout.preferredHeight: 16; color: Theme.chromeLine; Layout.leftMargin: 4; Layout.rightMargin: 4 }
                StatusButton { iconName: "zoom_out"; mirrorIcon: false; text: I18n.t("zoom.out"); shortcutHint: "Ctrl+-"; onClicked: view.zoomBy(0.8) }
                Slider {
                    id: zoomSlider
                    Layout.preferredWidth: tab.width > 900 ? 120 : 80
                    from: Math.log(0.1)
                    to: Math.log(8)
                    value: Math.log(Math.max(0.1, view.zoom))
                    onMoved: view.zoom = Math.exp(value)
                    Accessible.name: I18n.t("zoom.level")
                    background: Rectangle {
                        x: zoomSlider.leftPadding
                        y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                        width: zoomSlider.availableWidth
                        height: 2
                        color: Theme.controlBorder
                    }
                    handle: Rectangle {
                        x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                        y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                        implicitWidth: 8
                        implicitHeight: 14
                        radius: 2
                        color: zoomSlider.pressed ? Theme.accent : Theme.raised
                        border.width: zoomSlider.visualFocus ? 2 : 1
                        border.color: zoomSlider.visualFocus ? Theme.focusRing : Theme.ink
                    }
                }
                StatusButton { iconName: "zoom_in"; mirrorIcon: false; text: I18n.t("zoom.in"); shortcutHint: "Ctrl+="; onClicked: view.zoomBy(1.25) }
                AbstractButton {
                    id: percentButton
                    implicitWidth: 52
                    implicitHeight: 26
                    hoverEnabled: true
                    Accessible.role: Accessible.ButtonMenu
                    Accessible.name: I18n.t("zoom.percent", Math.round(view.zoom * 100))
                    onClicked: percentMenu.popup(percentButton, 0, -percentMenu.implicitHeight)
                    background: Rectangle { radius: 2; color: percentButton.hovered ? Theme.hover : "transparent" }
                    contentItem: Text {
                        text: I18n.isolate(Math.round(view.zoom * 100) + "%")
                        color: Theme.ink
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Menu {
                        id: percentMenu
                        Repeater {
                            model: [0.5, 0.75, 1, 1.25, 1.5, 2, 4]
                            delegate: MenuItem {
                                required property var modelData
                                text: I18n.isolate(Math.round(modelData * 100) + "%")
                                onTriggered: view.zoom = modelData
                            }
                        }
                        MenuSeparator {}
                        MenuItem { text: I18n.t("zoom.actualSize"); onTriggered: view.actualSize() }
                        MenuItem { text: I18n.t("zoom.fitPage"); onTriggered: view.fitMode = ReaderViewport.Page }
                        MenuItem { text: I18n.t("zoom.fitWidth"); onTriggered: view.fitMode = ReaderViewport.Width }
                    }
                }
                Rectangle { width: 1; Layout.preferredHeight: 16; color: Theme.chromeLine; Layout.leftMargin: 4; Layout.rightMargin: 4 }
                StatusButton { iconName: "full_screen_maximize"; mirrorIcon: false; text: I18n.t("cmd.fullScreen"); shortcutHint: "F11"; onClicked: AppState.fullScreen = true }
            }
        }
        }
    }

    // ---- status view: loading, password and errors
    ColumnLayout {
        visible: !tab.ready
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 56)
        spacing: Theme.md

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status === "loading"
            running: visible
        }
        IconImage {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status !== "loading"
            source: Theme.icon(tab.status === "passwordRequired" ? "lock_closed" : "warning")
            sourceSize.width: 40
            sourceSize.height: 40
            color: tab.status === "passwordRequired" ? Theme.accent : Theme.warning
        }
        Label {
            Layout.fillWidth: true
            text: tab.title
            color: Theme.ink
            font.pixelSize: 22
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
        }
        Label {
            Layout.fillWidth: true
            text: tab.status === "loading" ? I18n.t("reader.state.loading")
                : tab.status === "passwordRequired"
                    ? (tab.detail === "incorrect-password" ? I18n.t("reader.state.wrongPassword") : I18n.t("reader.state.password"))
                : tab.status === "missing" ? I18n.t("reader.state.missing")
                : tab.status === "unsupportedSecurity" ? I18n.t("reader.state.unsupportedSecurity")
                : I18n.t("reader.state.malformed")
            color: tab.detail === "incorrect-password" ? Theme.danger : Theme.muted
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(15 * I18n.sizeScale)
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
        AtlasTextField {
            id: passwordField
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(360, parent.width)
            visible: tab.status === "passwordRequired"
            placeholderText: I18n.t("reader.passwordPlaceholder")
            echoMode: TextInput.Password
            selectByMouse: true
            Accessible.name: I18n.t("reader.passwordPlaceholder")
            Accessible.description: I18n.t("reader.passwordNotSaved")
            onVisibleChanged: {
                if (visible) forceActiveFocus()
                else text = ""
            }
            onAccepted: {
                if (text.length === 0) return
                const submitted = text
                text = ""
                readerController.submitPassword(tab.index, submitted)
            }
        }
        AtlasButton {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status === "passwordRequired"
            enabled: passwordField.text.length > 0
            kind: "primary"
            iconName: "lock_closed"
            text: I18n.t("reader.unlock")
            Accessible.description: I18n.t("reader.passwordNotSaved")
            onClicked: {
                const submitted = passwordField.text
                passwordField.text = ""
                readerController.submitPassword(tab.index, submitted)
            }
        }
        Label {
            Layout.fillWidth: true
            visible: tab.status === "passwordRequired"
            text: I18n.t("reader.passwordNotSaved")
            color: Theme.muted
            font.pixelSize: Math.round(12 * I18n.sizeScale)
            horizontalAlignment: Text.AlignHCenter
        }
        Label {
            Layout.fillWidth: true
            text: I18n.ltr(tab.sourcePath)
            color: Theme.muted
            font.pixelSize: 12
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status !== "loading"
            AtlasButton {
                visible: tab.status !== "passwordRequired"
                kind: "primary"
                text: I18n.t("action.tryAgain")
                onClicked: readerController.retryAt(tab.index)
            }
            AtlasButton {
                text: I18n.t("tab.closeThis")
                onClicked: readerController.closeAt(tab.index)
            }
        }
    }

    // ---- page context menu (Foxit: right-click on the page)
    Menu {
        id: pageMenu
        property string link: ""
        MenuItem {
            visible: pageMenu.link.length > 0
            height: visible ? implicitHeight : 0
            text: I18n.t("link.copy")
            onTriggered: readerController.copyText(pageMenu.link)
        }
        MenuItem {
            visible: pageMenu.link.length > 0
            height: visible ? implicitHeight : 0
            text: I18n.t("link.open")
            onTriggered: tab.linkConfirmRequested(pageMenu.link)
        }
        MenuSeparator { visible: pageMenu.link.length > 0; height: visible ? implicitHeight : 0 }
        MenuItem {
            text: I18n.t("action.copy") + "\tCtrl+C"
            enabled: view.selectedText.length > 0
            onTriggered: view.copySelection()
        }
        MenuItem { text: I18n.t("action.selectAll") + "\tCtrl+A"; enabled: view.hasText; onTriggered: view.selectAllOnPage() }
        MenuSeparator {}
        MenuItem { text: I18n.t("cmd.hand") + "\tAlt+3"; checkable: true; checked: view.tool === ReaderViewport.HandTool; onTriggered: view.tool = ReaderViewport.HandTool }
        MenuItem { text: I18n.t("cmd.selectText") + "\tAlt+6"; checkable: true; checked: view.tool === ReaderViewport.SelectTool; onTriggered: view.tool = ReaderViewport.SelectTool }
        MenuItem { text: I18n.t("cmd.marqueeZoom"); checkable: true; checked: view.tool === ReaderViewport.MarqueeTool; onTriggered: view.tool = ReaderViewport.MarqueeTool }
        MenuItem { text: I18n.t("cmd.snapshot") + "\tAlt+7"; checkable: true; checked: view.tool === ReaderViewport.SnapshotTool; onTriggered: view.tool = ReaderViewport.SnapshotTool }
        MenuSeparator {}
        MenuItem { text: I18n.t("nav.previousPage"); enabled: view.currentPage > 1; onTriggered: view.previousPage() }
        MenuItem { text: I18n.t("nav.nextPage"); enabled: view.currentPage < view.pageCount; onTriggered: view.nextPage() }
        MenuItem { text: I18n.t("nav.previousView") + "\tAlt+Left"; enabled: view.canGoBack; onTriggered: view.goBack() }
        MenuSeparator {}
        MenuItem { text: I18n.t("find.title") + "\tCtrl+F"; onTriggered: tab.openFind() }
        MenuItem { text: I18n.t("panel.search") + "\tCtrl+Shift+F"; onTriggered: tab.openSearch() }
        MenuSeparator {}
        MenuItem { text: I18n.t("cmd.rotateClockwise"); onTriggered: view.rotateView(1) }
        MenuItem { text: I18n.t("cmd.rotateCounterclockwise"); onTriggered: view.rotateView(-1) }
    }
}
