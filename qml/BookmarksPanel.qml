import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Bookmarks panel (Foxit navigation pane → Bookmarks, N4.5): the document's
// own outline, read in the background. Top-level entries start collapsed;
// the entry for the current page is highlighted; click or Enter goes there.
Rectangle {
    id: panel

    required property var view          // ReaderViewport
    property var expanded: ({})         // outline index → true

    color: Theme.panel

    readonly property var outline: view ? view.outline : []

    // Indexes of the rows that are visible with the current expansion.
    readonly property var rows: {
        const result = []
        const items = panel.outline
        let hideBelow = -1                          // hide entries deeper than this level
        for (let i = 0; i < items.length; ++i) {
            const level = items[i].level
            if (hideBelow >= 0 && level > hideBelow) continue
            hideBelow = -1
            result.push(i)
            if (items[i].children > 0 && !panel.expanded[i]) hideBelow = level
        }
        return result
    }

    // Deepest outline entry at or before the current page.
    readonly property int currentEntry: {
        if (!view) return -1
        const page = view.currentPage - 1
        let best = -1
        for (let i = 0; i < outline.length; ++i)
            if (outline[i].page >= 0 && outline[i].page <= page) best = i
        return best
    }

    function toggle(index) {
        const copy = Object.assign({}, expanded)
        copy[index] = !copy[index]
        expanded = copy
    }
    function setAll(open) {
        const copy = {}
        if (open) for (let i = 0; i < outline.length; ++i) if (outline[i].children > 0) copy[i] = true
        expanded = copy
    }
    function activate(index) {
        const entry = outline[index]
        if (entry && entry.page >= 0) view.goToDestination(entry.page, entry.location)
    }

    onOutlineChanged: expanded = ({})

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.md
            Layout.rightMargin: Theme.xs
            Layout.topMargin: 2
            Layout.bottomMargin: 2
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: I18n.t("panel.bookmarks")
                color: Theme.ink
                font.pixelSize: Math.round(13 * I18n.sizeScale)
            }
            AtlasIconButton {
                implicitWidth: 26; implicitHeight: 26; iconSize: 16; cornerRadius: 2
                iconName: "text_bullet_list_tree"
                text: I18n.t("bookmarks.expandAll")
                enabled: panel.outline.length > 0
                onClicked: panel.setAll(true)
            }
            AtlasIconButton {
                implicitWidth: 26; implicitHeight: 26; iconSize: 16; cornerRadius: 2
                iconName: "text_bullet_list"
                text: I18n.t("bookmarks.collapseAll")
                enabled: panel.outline.length > 0
                onClicked: panel.setAll(false)
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.spacing }

        Label {
            Layout.fillWidth: true
            Layout.margins: Theme.md
            visible: panel.view && panel.view.textReady && panel.outline.length === 0
            text: I18n.t("bookmarks.none")
            color: Theme.muted
            wrapMode: Text.WordWrap
            font.pixelSize: Math.round(12 * I18n.sizeScale)
        }
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.md
            visible: panel.view && !panel.view.textReady
            running: visible
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            activeFocusOnTab: true
            keyNavigationEnabled: true
            model: panel.rows
            ScrollBar.vertical: ScrollBar {}
            Accessible.role: Accessible.Tree
            Accessible.name: I18n.t("panel.bookmarks")
            Keys.onReturnPressed: if (currentItem) panel.activate(currentItem.entryIndex)
            Keys.onRightPressed: if (currentItem && currentItem.hasChildren && !currentItem.open) panel.toggle(currentItem.entryIndex)
            Keys.onLeftPressed: if (currentItem && currentItem.open) panel.toggle(currentItem.entryIndex)

            delegate: AbstractButton {
                id: row
                required property int index
                required property var modelData
                readonly property int entryIndex: modelData
                readonly property var entry: panel.outline[entryIndex]
                readonly property bool hasChildren: entry.children > 0
                readonly property bool open: !!panel.expanded[entryIndex]
                readonly property bool current: entryIndex === panel.currentEntry
                width: ListView.view.width
                implicitHeight: Math.max(26, titleText.implicitHeight + 8)
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                Accessible.role: Accessible.TreeItem
                Accessible.name: entry.title
                Accessible.description: entry.page >= 0 ? I18n.t("panel.pageA11y", entry.page + 1) : ""
                Accessible.selected: current
                onClicked: { list.currentIndex = index; panel.activate(entryIndex) }
                onDoubleClicked: if (hasChildren) panel.toggle(entryIndex)

                background: Rectangle {
                    color: row.current ? Theme.accentSoft : row.hovered ? Theme.hover : "transparent"
                    border.width: list.activeFocus && row.ListView.isCurrentItem ? 1 : 0
                    border.color: Theme.focusRing
                }
                contentItem: Item {
                    AtlasIconButton {
                        id: arrow
                        readonly property real indent: 4 + row.entry.level * 14
                        x: row.mirrored ? parent.width - width - indent : indent
                        anchors.verticalCenter: parent.verticalCenter
                        implicitWidth: 18; implicitHeight: 18; iconSize: 12; cornerRadius: 2
                        visible: row.hasChildren
                        focusPolicy: Qt.NoFocus
                        iconName: row.open ? "chevron_down" : "chevron_right"
                        mirrorIcon: !row.open
                        text: row.open ? I18n.t("bookmarks.collapse") : I18n.t("bookmarks.expand")
                        onClicked: panel.toggle(row.entryIndex)
                    }
                    Text {
                        id: titleText
                        // anchors mirror in RTL; start after the arrow column either way
                        anchors.left: parent.left
                        anchors.leftMargin: arrow.indent + arrow.width + 4
                        anchors.right: parent.right
                        anchors.rightMargin: Theme.sm
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.entry.title
                        font.family: I18n.contentFamily(row.entry.title)
                        font.pixelSize: 13
                        color: Theme.ink
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        // Titles follow their own script direction.
                        textFormat: Text.PlainText
                    }
                }
                ToolTip.visible: hovered && titleText.truncated
                ToolTip.delay: 600
                ToolTip.text: entry.title
            }
        }
    }
}
