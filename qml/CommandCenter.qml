import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Command Center (Foxit title-bar search; N5.3). A magnifier in the title bar
// opens into a search field (Alt+Q or click). Results in three groups:
// commands (with their shortcuts), books in the library, and bookmarks in the
// open book. ↑/↓ move, Enter runs, Esc closes. Commands are matched by their
// label in the interface language and in English, so either works.
Item {
    id: center

    property bool hasDocument: false
    property var bookmarks: null          // BookmarkModel of the current tab, or null
    property bool canUndo: false
    property bool canRedo: false
    signal commandRequested(string command)
    signal bookRequested(string documentId, string path)
    signal bookmarkRequested(var found)

    readonly property bool active: field.visible
    implicitWidth: active ? Math.min(360, Math.max(220, parent ? parent.width * 0.32 : 300)) : button.implicitWidth
    implicitHeight: Theme.qatButton

    // id, label key, shortcut, icon, needs an open document
    readonly property var commands: [
        { id: "open", key: "action.openPdf", keys: "Ctrl+O", icon: "folder_open", doc: false },
        { id: "addFolder", key: "library.addFolder", keys: "Ctrl+Shift+O", icon: "folder_add", doc: false },
        { id: "library", key: "nav.library", keys: "", icon: "library", doc: false },
        { id: "preferences", key: "prefs.title", keys: "Ctrl+K", icon: "settings", doc: false },
        { id: "skins", key: "prefs.skins", keys: "", icon: "paint_brush", doc: false },
        { id: "about", key: "cmd.about", keys: "", icon: "info", doc: false },
        { id: "fullScreen", key: "cmd.fullScreen", keys: "F11", icon: "full_screen_maximize", doc: false },
        { id: "bookmark", key: "bookmarks.add", keys: "Ctrl+B", icon: "bookmark", doc: true },
        { id: "autoBookmarks", key: "autoBookmarks.title", keys: "", icon: "text_bullet_list_tree", doc: true },
        { id: "bookmarksPanel", key: "panel.bookmarks", keys: "", icon: "bookmark", doc: true },
        { id: "signaturesPanel", key: "panel.signatures", keys: "", icon: "signature", doc: true },
        { id: "pagesPanel", key: "panel.pages", keys: "", icon: "document_copy", doc: true },
        { id: "togglePanels", key: "panel.navigation", keys: "F4", icon: "panel_left", doc: true },
        { id: "find", key: "find.title", keys: "Ctrl+F", icon: "search", doc: true },
        { id: "search", key: "panel.search", keys: "Ctrl+Shift+F", icon: "search_square", doc: true },
        { id: "save", key: "save.qat", keys: "Ctrl+S", icon: "save", doc: true },
        { id: "undo", key: "bookmarks.undo", keys: "Ctrl+Z", icon: "arrow_undo", doc: true },
        { id: "redo", key: "bookmarks.redo", keys: "Ctrl+Y", icon: "arrow_redo", doc: true },
        { id: "goToPage", key: "nav.goToPage", keys: "Ctrl+G", icon: "document", doc: true },
        { id: "firstPage", key: "nav.firstPage", keys: "Ctrl+Home", icon: "arrow_previous", doc: true },
        { id: "lastPage", key: "nav.lastPage", keys: "End", icon: "arrow_next", doc: true },
        { id: "previousView", key: "nav.previousView", keys: "Alt+Left", icon: "arrow_left", doc: true },
        { id: "zoomIn", key: "zoom.in", keys: "Ctrl+=", icon: "zoom_in", doc: true },
        { id: "zoomOut", key: "zoom.out", keys: "Ctrl+-", icon: "zoom_out", doc: true },
        { id: "zoomTo", key: "zoom.zoomTo", keys: "Ctrl+M", icon: "zoom_in", doc: true },
        { id: "actualSize", key: "zoom.actualSize", keys: "Ctrl+1", icon: "ratio_one_to_one", doc: true },
        { id: "fitPage", key: "zoom.fitPage", keys: "Ctrl+0", icon: "page_fit", doc: true },
        { id: "fitWidth", key: "zoom.fitWidth", keys: "Ctrl+2", icon: "arrow_autofit_width", doc: true },
        { id: "fitVisible", key: "zoom.fitVisible", keys: "Ctrl+3", icon: "arrow_autofit_width", doc: true },
        { id: "rotateClockwise", key: "cmd.rotateClockwise", keys: "Ctrl+Shift++", icon: "arrow_rotate_clockwise", doc: true },
        { id: "rotateCounterclockwise", key: "cmd.rotateCounterclockwise", keys: "Ctrl+Shift+-", icon: "arrow_rotate_counterclockwise", doc: true },
        { id: "hand", key: "cmd.hand", keys: "Alt+3", icon: "hand_left", doc: true },
        { id: "selectText", key: "cmd.selectText", keys: "Alt+6", icon: "cursor", doc: true },
        { id: "snapshot", key: "cmd.snapshot", keys: "Alt+7", icon: "screenshot", doc: true },
        { id: "readMode", key: "cmd.readMode", keys: "Ctrl+H", icon: "book_open", doc: true },
        { id: "autoScroll", key: "cmd.autoScroll", keys: "Ctrl+Shift+H", icon: "arrow_down", doc: true },
        { id: "closeTab", key: "tab.closeThis", keys: "Ctrl+W", icon: "dismiss", doc: true },
        { id: "closeAll", key: "tab.closeAll", keys: "Ctrl+Shift+W", icon: "dismiss", doc: true },
        { id: "exit", key: "backstage.exit", keys: "Ctrl+Q", icon: "door_arrow_right", doc: false }
    ]

    // ---- matching (case and Arabic/Urdu marks ignored; word starts rank first)
    function fold(text) {
        return String(text).toLowerCase()
            .replace(/[ً-ٰٟۖ-ۭـ‌‍]/g, "")   // harakat, tatweel, joiners
            .replace(/[آأإٱ]/g, "ا")                         // alef forms
            .replace(/[ىیي]/g, "ي")                               // yeh forms
            .replace(/ک/g, "ك").replace(/ة/g, "ه")
    }
    function score(label, query) {
        const text = fold(label)
        if (text.startsWith(query)) return 3
        if (text.indexOf(" " + query) >= 0) return 2
        if (text.indexOf(query) >= 0) return 1
        return 0
    }
    function commandEnabled(command) {
        if (command.doc && !center.hasDocument) return false
        if (command.id === "undo") return center.canUndo
        if (command.id === "redo") return center.canRedo
        return true
    }

    property var results: []      // [{kind, title, detail, icon, enabled, payload}]
    property int current: 0
    property int requestId: 0

    function refresh() {
        const query = fold(field.text.trim())
        const rows = []
        ++requestId            // late book results for an older query are dropped
        bookRows = []
        current = 0
        if (query.length === 0) { results = []; return }
        const matches = []
        for (const command of commands) {
            const label = I18n.t(command.key)
            const s = Math.max(score(label, query), score(command.id, query))
            if (s > 0) matches.push({ s: s, command: command, label: label })
        }
        matches.sort((a, b) => b.s - a.s)
        for (const m of matches.slice(0, 8)) {
            rows.push({ kind: "command", title: m.label, detail: m.command.keys, icon: m.command.icon,
                        enabled: commandEnabled(m.command), payload: m.command.id })
        }
        if (center.bookmarks) {
            for (const b of center.bookmarks.findBookmarks(field.text, 6)) {
                rows.push({ kind: "bookmark", title: b.title, detail: b.breadcrumb, icon: "bookmark",
                            enabled: b.page >= 0, payload: b })
            }
        }
        results = rows
        while (current < rows.length - 1 && !rows[current].enabled) ++current
        libraryController.findBooks(field.text, requestId, 6)
    }
    property var bookRows: []
    Connections {
        target: libraryController
        function onBooksFound(id, books) {
            if (id !== center.requestId || !center.active) return
            const rows = center.results.filter(r => r.kind !== "book")
            center.bookRows = books.map(b => ({ kind: "book", title: b.title.length > 0 ? b.title : b.path.split(/[\\/]/).pop(),
                                                 detail: b.author, icon: "book", enabled: true, payload: b }))
            center.results = rows.concat(center.bookRows)
        }
    }

    function open() {
        field.visible = true
        field.text = ""
        bookRows = []
        results = []
        field.forceActiveFocus()
        popup.open()
    }
    function setQuery(text) { field.text = text }   // visual QA (--show command:<text>)
    function close() {
        popup.close()
        field.visible = false
        field.text = ""
    }
    function run(index) {
        const row = results[index]
        if (!row || !row.enabled) return
        close()
        if (row.kind === "command") center.commandRequested(row.payload)
        else if (row.kind === "book") center.bookRequested(row.payload.documentId, row.payload.path)
        else center.bookmarkRequested(row.payload)
    }
    function move(step) {
        if (results.length === 0) return
        let next = current
        for (let i = 0; i < results.length; ++i) {
            next = (next + step + results.length) % results.length
            if (results[next].enabled) break
        }
        current = next
        list.positionViewAtIndex(current, ListView.Contain)
    }

    AtlasIconButton {
        id: button
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        visible: !center.active
        implicitWidth: Theme.qatButton
        implicitHeight: Theme.qatButton
        iconSize: 16
        iconName: "search"
        iconColor: Theme.titleBarText
        hoverColor: Theme.titleBarHover
        cornerRadius: 2
        text: I18n.t("commandCenter.open")
        shortcutHint: "Alt+Q"
        onClicked: center.open()
    }

    TextField {
        id: field
        visible: false
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        // The magnifier sits at the start of the field in either direction.
        leftPadding: I18n.rtl ? Theme.sm : 30
        rightPadding: I18n.rtl ? 30 : Theme.sm
        placeholderText: I18n.t("commandCenter.placeholder")
        placeholderTextColor: Theme.muted
        color: Theme.ink
        selectionColor: Theme.accent
        selectedTextColor: Theme.accentText
        font.family: I18n.contentFamily(text) || I18n.uiFamily
        font.pixelSize: Math.round(12 * I18n.sizeScale)
        verticalAlignment: TextInput.AlignVCenter
        Accessible.role: Accessible.EditableText
        Accessible.name: I18n.t("commandCenter.open")
        background: Rectangle {
            radius: Theme.radiusSmall
            color: Theme.raised
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.focusRing : Theme.controlBorder
        }
        IconImage {
            x: I18n.rtl ? field.width - width - 9 : 9
            anchors.verticalCenter: parent.verticalCenter
            source: Theme.icon("search")
            sourceSize.width: 14; sourceSize.height: 14
            color: Theme.muted
        }
        onTextChanged: center.refresh()
        Keys.onDownPressed: center.move(1)
        Keys.onUpPressed: center.move(-1)
        Keys.onReturnPressed: center.run(center.current)
        Keys.onEnterPressed: center.run(center.current)
        Keys.onEscapePressed: center.close()
        onActiveFocusChanged: if (!activeFocus && popup.opened) closeTimer.start()
    }
    Timer { id: closeTimer; interval: 150; onTriggered: if (!field.activeFocus) center.close() }

    Popup {
        id: popup
        y: center.height + 4
        // The center sits at the far end of the title bar, so the list opens inward.
        x: I18n.rtl ? 0 : center.width - width
        width: Math.max(center.width, 420)
        height: Math.min(contentColumn.implicitHeight + 2 * padding, 460)
        padding: 4
        closePolicy: Popup.CloseOnPressOutsideParent
        onClosed: if (field.visible && !field.activeFocus) center.close()
        background: Rectangle {
            color: Theme.raised
            radius: Theme.radiusControl
            border.color: Theme.chromeLine
        }
        contentItem: ColumnLayout {
            id: contentColumn
            spacing: 0
            Label {
                Layout.fillWidth: true
                Layout.margins: Theme.sm
                visible: center.results.length === 0
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                color: Theme.muted
                font.pixelSize: Math.round(12 * I18n.sizeScale)
                text: field.text.trim().length === 0 ? I18n.t("commandCenter.hint") : I18n.t("commandCenter.none")
            }
            ListView {
                id: list
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, 440)
                visible: count > 0
                clip: true
                model: center.results
                currentIndex: center.current
                interactive: contentHeight > height
                Accessible.role: Accessible.List
                Accessible.name: I18n.t("commandCenter.results")
                delegate: Column {
                    id: entry
                    required property int index
                    required property var modelData
                    width: ListView.view.width
                    // Group heading above the first row of each kind.
                    Label {
                        visible: entry.index === 0 || center.results[entry.index - 1].kind !== entry.modelData.kind
                        width: parent.width
                        topPadding: entry.index === 0 ? 2 : Theme.sm
                        bottomPadding: 2
                        leftPadding: Theme.sm
                        rightPadding: Theme.sm
                        horizontalAlignment: Text.AlignLeft
                        text: I18n.t(entry.modelData.kind === "command" ? "commandCenter.commands"
                                   : entry.modelData.kind === "book" ? "commandCenter.books" : "commandCenter.bookmarks")
                        color: Theme.muted
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(11 * I18n.sizeScale)
                        font.bold: true
                        Accessible.role: Accessible.Heading
                    }
                AbstractButton {
                    id: row
                    readonly property int index: entry.index
                    readonly property var modelData: entry.modelData
                    width: parent.width
                    implicitHeight: Math.max(30, rowLayout.implicitHeight + 8)
                    enabled: modelData.enabled
                    hoverEnabled: true
                    focusPolicy: Qt.NoFocus
                    Accessible.role: Accessible.ListItem
                    Accessible.name: modelData.title
                    Accessible.description: modelData.detail || ""
                    Accessible.selected: index === center.current
                    onHoveredChanged: if (hovered && enabled) center.current = index
                    onClicked: center.run(index)
                    background: Rectangle {
                        radius: Theme.radiusSmall
                        color: row.index === center.current ? Theme.accentSoft : "transparent"
                    }
                    contentItem: RowLayout {
                        id: rowLayout
                        spacing: Theme.sm
                        IconImage {
                            Layout.leftMargin: Theme.sm
                            source: Theme.icon(row.modelData.icon)
                            sourceSize.width: 16; sourceSize.height: 16
                            color: row.enabled ? Theme.ink : Qt.alpha(Theme.ink, 0.38)
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Text {
                                Layout.fillWidth: true
                                text: row.modelData.title
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignLeft
                                color: row.enabled ? Theme.ink : Qt.alpha(Theme.ink, 0.38)
                                font.family: I18n.contentFamily(row.modelData.title) || I18n.uiFamily
                                font.pixelSize: Math.round(13 * I18n.sizeScale)
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: row.modelData.kind !== "command" && (row.modelData.detail || "").length > 0
                                text: row.modelData.detail || ""
                                textFormat: Text.PlainText
                                elide: Text.ElideMiddle
                                horizontalAlignment: Text.AlignLeft
                                color: Theme.muted
                                font.family: I18n.contentFamily(text) || I18n.uiFamily
                                font.pixelSize: Math.round(11 * I18n.sizeScale)
                            }
                        }
                        Text {   // shortcut: keys read left to right in every language
                            Layout.rightMargin: Theme.sm
                            visible: row.modelData.kind === "command" && row.modelData.detail.length > 0
                            text: row.modelData.detail
                            color: Theme.muted
                            font.pixelSize: 11
                            LayoutMirroring.enabled: false
                        }
                    }
                }
                }
            }
        }
    }
}
