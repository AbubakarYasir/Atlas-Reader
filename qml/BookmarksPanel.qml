import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Atlas.Reader

// Bookmarks panel (Foxit navigation pane → Bookmarks; N5.3). The book's own
// outline plus the reader's bookmarks, in one tree. Click or Enter goes there;
// everything else is on the keyboard too (DESIGN.md §6):
//   F2 rename · Delete delete (PDF bookmarks are hidden instead) ·
//   Alt+Up/Down move · Alt+Right/Left nest/un-nest (mirrored in RTL) ·
//   Right/Left expand/collapse (mirrored in RTL) · Ctrl+Z / Ctrl+Y undo/redo ·
//   Shift+F10 or the Menu key for the bookmark menu.
// Bookmarks that differ from the PDF carry a small icon with the reason in
// words (tooltip and screen reader); colour is never the only signal.
Rectangle {
    id: panel

    required property var view          // ReaderViewport
    required property var bookmarks     // BookmarkModel
    signal toastRequested(string text)

    color: Theme.panel

    readonly property bool mirrored: I18n.rtl
    readonly property bool canEdit: bookmarks.editable
    readonly property int rowHeight: Theme.touch ? 34 : 26
    property string editingId: ""

    // ---- actions
    function rowData(row) { return row >= 0 ? bookmarks.get(row) : ({}) }
    function currentRow() { return list.currentIndex }

    function defaultTitle() {
        const selected = view.selectedText.replace(/\s+/g, " ").trim()
        if (selected.length > 0 && selected.length <= 200) return selected
        const loc = view.viewLocation()
        const page = loc.page !== undefined ? loc.page + 1 : view.currentPage
        const labels = view.pageLabels
        return I18n.t("bookmarks.untitled", page >= 1 && page <= labels.length ? labels[page - 1] : page)
    }
    // Ctrl+B and the Bookmark buttons: a bookmark for what the reader shows now.
    function addHere(asChild) {
        if (!canEdit) { toastRequested(I18n.t("bookmarks.readOnly." + (bookmarks.statusReason || "storageError"))); return }
        bookmarks.filter = ""
        const row = bookmarks.addBookmark(currentRow(), asChild, defaultTitle(), view.viewLocation())
        if (row >= 0) {
            list.currentIndex = row
            list.positionViewAtIndex(row, ListView.Contain)
            startRename(row)
        }
    }
    function go(row) {
        const data = rowData(row)
        if (data.page === undefined || data.page < 0) return
        view.goToNormalized(data.page, data.fx, data.fy)
    }
    function startRename(row) {
        if (!canEdit || row < 0) return
        const data = rowData(row)
        if (data.storage === "locally_hidden" || data.storage === "conflict") {
            toastRequested(I18n.t(data.storage === "conflict" ? "bookmarks.refused.resolveFirst" : "bookmarks.refused.unhideFirst"))
            return
        }
        list.currentIndex = row
        editingId = data.nodeId
    }
    function finishRename(id, text, accept) {
        if (editingId !== id) return
        editingId = ""
        const row = bookmarks.rowOf(id)
        if (accept && row >= 0 && text.length > 0) select(bookmarks.rename(row, text))
        list.forceActiveFocus()
    }
    function select(row) {
        if (row < 0) return
        list.currentIndex = row
        list.positionViewAtIndex(row, ListView.Contain)
    }
    function removeCurrent() {
        const row = currentRow()
        if (row < 0) return
        select(bookmarks.remove(row))
    }
    function openMenu(row, x, y, item) {
        if (row >= 0) list.currentIndex = row
        rowMenu.row = row
        rowMenu.popup(item || list, x, y)
    }

    Connections {
        target: panel.bookmarks
        function onRefused(reason) {
            if (reason === "busy" || reason === "unchanged" || reason === "nothingToUndo" || reason === "nothingToRedo") return
            const known = ["readOnly", "atTop", "atBottom", "atTopLevel", "nothingToIndentInto", "unhideFirst",
                           "resolveFirst", "containsSourceNodes", "cycle", "tooDeep", "tooManyNodes", "badTitle",
                           "historyStale"]
            panel.toastRequested(I18n.t("bookmarks.refused." + (known.indexOf(reason) >= 0 ? reason : "other")))
        }
        function onReloaded() { panel.toastRequested(I18n.t("bookmarks.reloaded")) }
        // The field always shows the filter, however it was set.
        function onFilterChanged() { if (findField.text !== panel.bookmarks.filter) findField.text = panel.bookmarks.filter }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- header: title and commands (Foxit bookmark pane toolbar)
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.md
            Layout.rightMargin: Theme.xs
            Layout.topMargin: 2
            Layout.bottomMargin: 2
            spacing: 0
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: I18n.t("panel.bookmarks")
                color: Theme.ink
                elide: Text.ElideRight
                font.family: I18n.uiFamily
                font.pixelSize: Math.round(13 * I18n.sizeScale)
            }
            AtlasIconButton {
                implicitWidth: 26; implicitHeight: 26; iconSize: 16; cornerRadius: 2
                iconName: "bookmark_add"
                text: I18n.t("bookmarks.add")
                shortcutHint: "Ctrl+B"
                enabled: panel.canEdit
                onClicked: panel.addHere(false)
            }
            AtlasIconButton {
                implicitWidth: 26; implicitHeight: 26; iconSize: 16; cornerRadius: 2
                iconName: "delete"
                text: I18n.t("bookmarks.delete")
                shortcutHint: "Delete"
                enabled: panel.canEdit && list.currentIndex >= 0
                onClicked: panel.removeCurrent()
            }
            AtlasIconButton {   // one button: collapse when anything is open, else expand
                implicitWidth: 26; implicitHeight: 26; iconSize: 16; cornerRadius: 2
                iconName: panel.bookmarks.anyExpanded ? "text_bullet_list" : "text_bullet_list_tree"
                text: panel.bookmarks.anyExpanded ? I18n.t("bookmarks.collapseAll") : I18n.t("bookmarks.expandAll")
                enabled: panel.bookmarks.totalCount > 0 && panel.bookmarks.filter.length === 0
                onClicked: panel.bookmarks.anyExpanded ? panel.bookmarks.collapseAll() : panel.bookmarks.expandAll()
            }
            AtlasIconButton {
                id: optionsButton
                implicitWidth: 26; implicitHeight: 26; iconSize: 16; cornerRadius: 2
                iconName: "more_horizontal"
                text: I18n.t("bookmarks.options")
                enabled: panel.bookmarks.status !== "none"
                onClicked: panel.openMenu(list.currentIndex, 0, height, optionsButton)
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.spacing }

        // ---- find
        AtlasTextField {
            id: findField
            Layout.fillWidth: true
            Layout.margins: Theme.sm
            implicitHeight: (Theme.touch ? 36 : 28) + Theme.scriptExtra
            visible: panel.bookmarks.totalCount > 0
            placeholderText: I18n.t("bookmarks.find")
            Accessible.name: I18n.t("bookmarks.find")
            font.family: I18n.contentFamily(text) || I18n.uiFamily
            font.pixelSize: Math.round(13 * I18n.sizeScale)
            onTextChanged: panel.bookmarks.filter = text
            Keys.onEscapePressed: { text = ""; list.forceActiveFocus() }
            Keys.onDownPressed: { list.forceActiveFocus(); if (list.count > 0) list.currentIndex = 0 }
            Keys.onReturnPressed: if (list.count > 0) panel.go(0)
        }

        // ---- notices
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sm
            Layout.rightMargin: Theme.sm
            Layout.bottomMargin: Theme.sm
            visible: notice.text.length > 0
            implicitHeight: noticeRow.implicitHeight + Theme.sm * 2
            radius: Theme.radiusSmall
            color: panel.bookmarks.outlineChangedOutside ? Theme.warningSoft : Theme.rail
            RowLayout {
                id: noticeRow
                anchors.fill: parent
                anchors.margins: Theme.sm
                spacing: Theme.sm
                IconImage {
                    Layout.alignment: Qt.AlignTop
                    source: Theme.icon(panel.bookmarks.outlineChangedOutside ? "warning" : "info")
                    sourceSize.width: 16; sourceSize.height: 16
                    color: panel.bookmarks.outlineChangedOutside ? Theme.warning : Theme.muted
                }
                Label {
                    id: notice
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignLeft
                    color: Theme.ink
                    font.family: I18n.uiFamily
                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                    text: panel.bookmarks.status === "readOnly"
                          ? I18n.t("bookmarks.readOnly." + (panel.bookmarks.statusReason || "storageError"))
                          : panel.bookmarks.outlineChangedOutside ? I18n.t("bookmarks.outlineChanged") : ""
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.margins: Theme.md
            visible: text.length > 0
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            color: Theme.muted
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(12 * I18n.sizeScale)
            text: panel.bookmarks.status === "loading" ? ""
                  : panel.bookmarks.totalCount === 0
                    ? (panel.canEdit ? I18n.t("bookmarks.empty") : I18n.t("bookmarks.none"))
                  : panel.bookmarks.filter.length > 0 && list.count === 0 ? I18n.t("bookmarks.noMatches") : ""
        }
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.md
            visible: panel.bookmarks.status === "loading" || (panel.view && !panel.view.textReady)
            running: visible
        }

        // ---- the tree
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            activeFocusOnTab: true
            keyNavigationEnabled: true
            highlightMoveDuration: 0
            model: panel.bookmarks
            currentIndex: -1
            ScrollBar.vertical: ScrollBar {}
            Accessible.role: Accessible.Tree
            Accessible.name: I18n.t("panel.bookmarks")

            onActiveFocusChanged: if (activeFocus && currentIndex < 0 && count > 0) currentIndex = 0

            Keys.onPressed: (event) => {
                const row = currentIndex
                const alt = event.modifiers & Qt.AltModifier
                const ctrl = event.modifiers & Qt.ControlModifier
                const shift = event.modifiers & Qt.ShiftModifier
                const forward = panel.mirrored ? Qt.Key_Left : Qt.Key_Right   // deeper
                const back = panel.mirrored ? Qt.Key_Right : Qt.Key_Left       // shallower
                const data = panel.rowData(row)
                let handled = true
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) panel.go(row)
                else if (event.key === Qt.Key_F2) panel.startRename(row)
                else if (event.key === Qt.Key_Delete) panel.removeCurrent()
                else if (alt && event.key === Qt.Key_Up) panel.select(panel.bookmarks.moveUp(row))
                else if (alt && event.key === Qt.Key_Down) panel.select(panel.bookmarks.moveDown(row))
                else if (alt && event.key === forward) panel.select(panel.bookmarks.indent(row))
                else if (alt && event.key === back) panel.select(panel.bookmarks.outdent(row))
                else if (!alt && event.key === forward) {
                    if (data.hasChildren && !data.expanded) panel.bookmarks.setExpanded(row, true)
                    else if (data.hasChildren) currentIndex = row + 1
                } else if (!alt && event.key === back) {
                    if (data.expanded) panel.bookmarks.setExpanded(row, false)
                    else {   // go to the parent row
                        for (let r = row - 1; r >= 0; --r)
                            if (panel.rowData(r).depth < data.depth) { currentIndex = r; break }
                    }
                }
                else if (event.key === Qt.Key_Home) currentIndex = 0
                else if (event.key === Qt.Key_End) currentIndex = count - 1
                else if (ctrl && event.key === Qt.Key_Z && !shift) panel.bookmarks.undo()
                else if (ctrl && (event.key === Qt.Key_Y || (shift && event.key === Qt.Key_Z))) panel.bookmarks.redo()
                else if (event.key === Qt.Key_Menu || (shift && event.key === Qt.Key_F10)) {
                    const item = itemAtIndex(row)
                    panel.openMenu(row, item ? item.width / 2 : 0, item ? item.height : 0, item)
                }
                else handled = false
                event.accepted = handled
            }

            delegate: AbstractButton {
                id: row
                required property int index
                required property string nodeId
                required property string title
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property string storage
                required property int page
                required property var breadcrumb
                required property string match
                required property bool fromPdf

                readonly property bool current: nodeId === panel.bookmarks.currentId
                readonly property bool editing: nodeId === panel.editingId
                readonly property bool marked: storage !== "embedded"
                readonly property bool filtering: panel.bookmarks.filter.length > 0
                readonly property real indent: 4 + depth * 14
                // Arrow and icon centre on the title's first line (Nastaliq lines are taller).
                readonly property real lineCenter: editing ? panel.rowHeight / 2
                                                           : 3 + Math.max(16, titleText.implicitHeight) / 2

                width: ListView.view.width
                implicitHeight: Math.max(panel.rowHeight, column.implicitHeight + 6)
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                Accessible.role: Accessible.TreeItem
                Accessible.name: title
                // Level, open/closed, page and storage state, for screen readers.
                Accessible.description: [
                    I18n.t("bookmarks.levelA11y", depth + 1),
                    hasChildren ? I18n.t(expanded ? "bookmarks.expandedA11y" : "bookmarks.collapsedA11y") : "",
                    page >= 0 ? I18n.t("panel.pageA11y", page + 1) : "",
                    marked ? I18n.t("bookmarks.state." + storage) : ""
                ].filter(s => s.length > 0).join(", ")
                Accessible.selected: ListView.isCurrentItem

                onClicked: { list.currentIndex = index; list.forceActiveFocus(); panel.go(index) }
                onDoubleClicked: if (hasChildren) panel.bookmarks.toggle(index)
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: (point) => panel.openMenu(row.index, point.position.x, point.position.y, row)
                }

                background: Rectangle {
                    color: row.ListView.isCurrentItem && list.activeFocus ? Theme.accentSoft
                         : row.current ? Theme.accentSoft
                         : row.hovered ? Theme.hover : "transparent"
                    border.width: list.activeFocus && row.ListView.isCurrentItem ? 1 : 0
                    border.color: Theme.focusRing
                    Rectangle {   // current-page marker: shape, not colour alone
                        visible: row.current
                        width: 3
                        height: parent.height
                        color: Theme.accent
                        x: panel.mirrored ? parent.width - width : 0
                    }
                }

                contentItem: Item {
                    AtlasIconButton {
                        id: arrow
                        x: panel.mirrored ? parent.width - width - row.indent : row.indent
                        y: row.lineCenter - height / 2
                        implicitWidth: 18; implicitHeight: 18; iconSize: 12; cornerRadius: 2
                        visible: row.hasChildren
                        focusPolicy: Qt.NoFocus
                        iconName: row.expanded ? "chevron_down" : "chevron_right"
                        mirrorIcon: !row.expanded
                        text: row.expanded ? I18n.t("bookmarks.collapse") : I18n.t("bookmarks.expand")
                        onClicked: panel.bookmarks.toggle(row.index)
                    }
                    IconImage {
                        id: stateIcon
                        x: panel.mirrored ? parent.width - width - row.indent - 20 : row.indent + 20
                        y: row.lineCenter - height / 2
                        visible: row.marked
                        width: visible ? 14 : 0
                        height: 14
                        sourceSize.width: 14; sourceSize.height: 14
                        source: !row.marked ? ""
                              : Theme.icon(row.storage === "local_only" ? "desktop"
                                         : row.storage === "pending_embed" ? "arrow_sync"
                                         : row.storage === "local_override" ? "edit"
                                         : row.storage === "locally_hidden" ? "eye_off" : "warning")
                        color: row.storage === "conflict" ? Theme.warning : Theme.muted
                        HoverHandler { id: stateHover }
                        ToolTip.visible: stateHover.hovered
                        ToolTip.delay: 300
                        ToolTip.text: row.marked ? I18n.t("bookmarks.state." + row.storage) : ""
                    }
                    Column {
                        id: column
                        readonly property real start: row.indent + 20 + (row.marked ? 18 : 0)
                        anchors.left: parent.left
                        anchors.leftMargin: start
                        anchors.right: parent.right
                        anchors.rightMargin: Theme.sm
                        y: row.editing ? (panel.rowHeight - titleEdit.height) / 2 : 3
                        spacing: 0
                        Text {
                            id: titleText
                            width: parent.width
                            visible: !row.editing
                            text: row.title
                            textFormat: Text.PlainText
                            font.family: I18n.contentFamily(row.title)
                            font.pixelSize: 13
                            font.italic: row.storage === "locally_hidden"
                            color: row.storage === "locally_hidden" ? Theme.muted : Theme.ink
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft   // each title keeps its own script direction
                        }
                        Text {
                            width: parent.width
                            visible: row.filtering && !row.editing && (row.breadcrumb.length > 0 || row.match === "notes" || row.match === "tag")
                            text: {
                                const parts = []
                                if (row.breadcrumb.length > 0) parts.push(row.breadcrumb.join(" › "))
                                if (row.match === "notes") parts.push(I18n.t("bookmarks.matchNotes"))
                                if (row.match === "tag") parts.push(I18n.t("bookmarks.matchTag"))
                                return parts.join(" · ")
                            }
                            textFormat: Text.PlainText
                            font.family: I18n.contentFamily(text) || I18n.uiFamily
                            font.pixelSize: 11
                            color: Theme.muted
                            elide: Text.ElideMiddle
                            horizontalAlignment: Text.AlignLeft
                        }
                        TextField {
                            id: titleEdit
                            width: parent.width
                            visible: row.editing
                            height: panel.rowHeight - 2
                            padding: 2
                            leftPadding: 4
                            rightPadding: 4
                            color: Theme.ink
                            selectionColor: Theme.accent
                            selectedTextColor: Theme.accentText
                            font.family: I18n.contentFamily(text)
                            font.pixelSize: 13
                            Accessible.name: I18n.t("bookmarks.renameField")
                            background: Rectangle { color: Theme.raised; border.width: 2; border.color: Theme.focusRing; radius: 2 }
                            onVisibleChanged: if (visible) { text = row.title; selectAll(); forceActiveFocus() }
                            Keys.onReturnPressed: panel.finishRename(row.nodeId, text, true)
                            Keys.onEnterPressed: panel.finishRename(row.nodeId, text, true)
                            Keys.onEscapePressed: panel.finishRename(row.nodeId, text, false)
                            onActiveFocusChanged: if (!activeFocus && row.editing) panel.finishRename(row.nodeId, text, true)
                        }
                    }
                }
                ToolTip.visible: hovered && titleText.truncated && !stateHover.hovered
                ToolTip.delay: 600
                ToolTip.text: title
            }
        }

        // ---- hidden bookmarks count (quiet; only when some are hidden)
        AbstractButton {
            Layout.fillWidth: true
            visible: panel.bookmarks.hiddenCount > 0
            implicitHeight: Math.max(24, hiddenLabel.implicitHeight + 6)
            hoverEnabled: true
            Accessible.role: Accessible.CheckBox
            Accessible.name: I18n.t("bookmarks.showHidden")
            Accessible.checked: panel.bookmarks.showHidden
            onClicked: panel.bookmarks.showHidden = !panel.bookmarks.showHidden
            background: Rectangle { color: parent.hovered ? Theme.hover : "transparent" }
            contentItem: RowLayout {
                spacing: Theme.xs
                IconImage {
                    Layout.leftMargin: Theme.md
                    source: Theme.icon(panel.bookmarks.showHidden ? "eye" : "eye_off")
                    sourceSize.width: 14; sourceSize.height: 14
                    color: Theme.muted
                }
                Label {
                    id: hiddenLabel
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    text: panel.bookmarks.hiddenCount === 1 ? I18n.t("bookmarks.hiddenCount.one", 1)
                                                              : I18n.t("bookmarks.hiddenCount.other", panel.bookmarks.hiddenCount)
                    color: Theme.muted
                    font.family: I18n.uiFamily
                    font.pixelSize: Math.round(11 * I18n.sizeScale)
                    elide: Text.ElideRight
                }
            }
        }
    }

    // ---- bookmark menu (right-click, Shift+F10, Menu key, Options button)
    Menu {
        id: rowMenu
        property int row: -1
        readonly property var data: row >= 0 ? panel.rowData(row) : ({})
        readonly property bool hasRow: row >= 0 && data.nodeId !== undefined
        readonly property bool frozen: data.storage === "locally_hidden" || data.storage === "conflict"
        // As wide as the longest command, so nothing is cut off in any language.
        width: {
            let widest = 0
            for (let i = 0; i < count; ++i) {
                const item = itemAt(i)
                if (item && item.visible) widest = Math.max(widest, item.implicitWidth)
            }
            return Math.min(Math.max(200, widest + leftPadding + rightPadding), 480)
        }

        MenuItem { text: I18n.t("bookmarks.goTo"); enabled: rowMenu.hasRow && rowMenu.data.page >= 0; onTriggered: panel.go(rowMenu.row) }
        MenuSeparator {}
        MenuItem { text: I18n.t("bookmarks.add"); enabled: panel.canEdit; onTriggered: panel.addHere(false) }
        MenuItem { text: I18n.t("bookmarks.addChild"); enabled: panel.canEdit && rowMenu.hasRow && !rowMenu.frozen; onTriggered: panel.addHere(true) }
        MenuItem { text: I18n.t("bookmarks.rename"); enabled: panel.canEdit && rowMenu.hasRow && !rowMenu.frozen; onTriggered: panel.startRename(rowMenu.row) }
        MenuItem { text: I18n.t("bookmarks.setDestination"); enabled: panel.canEdit && rowMenu.hasRow && !rowMenu.frozen; onTriggered: panel.select(panel.bookmarks.setDestination(rowMenu.row, panel.view.viewLocation())) }
        MenuItem {
            text: rowMenu.data.fromPdf ? I18n.t("bookmarks.hide") : I18n.t("bookmarks.delete")
            visible: !(rowMenu.data.storage === "locally_hidden")
            height: visible ? implicitHeight : 0
            enabled: panel.canEdit && rowMenu.hasRow && rowMenu.data.storage !== "conflict"
            onTriggered: panel.select(panel.bookmarks.remove(rowMenu.row))
        }
        MenuItem {
            text: I18n.t("bookmarks.unhide")
            visible: rowMenu.data.storage === "locally_hidden"
            height: visible ? implicitHeight : 0
            enabled: panel.canEdit
            onTriggered: panel.select(panel.bookmarks.setHidden(rowMenu.row, false))
        }
        MenuSeparator {}
        MenuItem { text: I18n.t("bookmarks.moveUp"); enabled: panel.canEdit && rowMenu.hasRow && panel.bookmarks.filter.length === 0; onTriggered: panel.select(panel.bookmarks.moveUp(rowMenu.row)) }
        MenuItem { text: I18n.t("bookmarks.moveDown"); enabled: panel.canEdit && rowMenu.hasRow && panel.bookmarks.filter.length === 0; onTriggered: panel.select(panel.bookmarks.moveDown(rowMenu.row)) }
        MenuItem { text: I18n.t("bookmarks.indent"); enabled: panel.canEdit && rowMenu.hasRow && panel.bookmarks.filter.length === 0; onTriggered: panel.select(panel.bookmarks.indent(rowMenu.row)) }
        MenuItem { text: I18n.t("bookmarks.outdent"); enabled: panel.canEdit && rowMenu.hasRow && rowMenu.data.depth > 0 && panel.bookmarks.filter.length === 0; onTriggered: panel.select(panel.bookmarks.outdent(rowMenu.row)) }
        MenuSeparator {}
        MenuItem { text: I18n.t("bookmarks.undo"); enabled: panel.bookmarks.canUndo; onTriggered: panel.bookmarks.undo() }
        MenuItem { text: I18n.t("bookmarks.redo"); enabled: panel.bookmarks.canRedo; onTriggered: panel.bookmarks.redo() }
        MenuSeparator {}
        MenuItem {
            text: I18n.t("bookmarks.showHidden")
            checkable: true
            checked: panel.bookmarks.showHidden
            enabled: panel.bookmarks.hiddenCount > 0 || panel.bookmarks.showHidden
            onTriggered: panel.bookmarks.showHidden = checked
        }
    }
}
