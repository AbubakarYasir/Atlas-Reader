import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Auto Create Bookmarks (Foxit: Bookmarks panel → Auto Create Bookmarks, N5.3).
// Atlas reads the whole book in the background and suggests headings by their
// type size and wording; the reader checks the preview, chooses how many
// levels and where the new bookmarks go, then creates them in one step that
// a single Undo takes back. Nothing is written into the PDF.
Popup {
    id: dialog

    required property var view          // ReaderViewport
    required property var bookmarks     // BookmarkModel
    property var pageLabel: (page) => String(page)
    signal created(int count)

    property var headings: []           // all suggestions (scanned with 4 levels)
    property var checks: []             // parallel to headings
    property int levels: 3
    property string placement: "after"
    property bool scanning: false
    property int pagesDone: 0
    property int pageCount: 0
    property bool truncated: false
    property bool qaCreate: false       // visual QA: create as soon as the scan ends
    readonly property var rows: {       // indices shown for the chosen levels
        const out = []
        for (let i = 0; i < headings.length; ++i) if (headings[i].level < levels) out.push(i)
        return out
    }
    readonly property int chosen: {
        let n = 0
        for (const i of rows) if (checks[i]) ++n
        return n
    }

    property var existing: []
    function start() {
        headings = []
        existing = []
        checks = []
        truncated = false
        pagesDone = 0
        pageCount = view ? view.pageCount : 0
        scanning = true
        open()
        view.scanHeadings(4)
    }
    function setAll(value) {
        const next = checks.slice()
        for (const i of rows) next[i] = value
        checks = next
    }
    function toggle(index) {
        const next = checks.slice()
        next[index] = !next[index]
        checks = next
    }
    function create() {
        const picked = []
        for (const i of rows) if (checks[i]) picked.push(headings[i])
        if (picked.length === 0) return
        const row = bookmarks.addMany(picked, placement)
        if (row >= 0) {
            created(picked.length)
            close()
        }
    }

    Connections {
        target: dialog.view
        ignoreUnknownSignals: true
        function onHeadingsProgress(done, count) { dialog.pagesDone = done; dialog.pageCount = count }
        function onHeadingsReady(found, bodySize, truncated) {
            if (!dialog.scanning) return
            dialog.headings = found
            // Headings that are already bookmarks start unchecked, so running
            // Auto Create again does not add duplicates.
            dialog.existing = bookmarks ? bookmarks.alreadyBookmarked(found) : found.map(() => false)
            dialog.checks = found.map((heading, i) => dialog.existing[i] !== true)
            dialog.truncated = truncated
            dialog.scanning = false
            if (dialog.qaCreate) { dialog.qaCreate = false; dialog.create() }
            Qt.callLater(() => preview.positionViewAtBeginning()) // the header grew; show it from the top
        }
    }
    onClosed: if (scanning) { scanning = false; view.cancelHeadingScan() }

    modal: true
    focus: true
    parent: Overlay.overlay
    x: Math.round(((parent ? parent.width : 800) - width) / 2)
    y: Math.round(((parent ? parent.height : 600) - height) / 2)
    width: Math.min(560, (parent ? parent.width : 800) - 40)
    // Tall enough for the options plus a few headings in Urdu's Nastaliq too.
    height: Math.min(Math.round(760 * Math.max(1, I18n.sizeScale)), (parent ? parent.height : 600) - 40)
    padding: Theme.xl
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: Theme.raised; radius: Theme.radiusSurface; border.color: Theme.chromeLine }

    contentItem: ColumnLayout {
        spacing: Theme.md
        Accessible.role: Accessible.Dialog
        Accessible.name: I18n.t("autoBookmarks.title")

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            text: I18n.t("autoBookmarks.title")
            color: Theme.ink
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(16 * I18n.sizeScale)
            font.weight: Font.DemiBold
        }
        // One scrolling list: the options and progress ride in its header, so
        // the dialog fits small windows, large text and Nastaliq at any scale.
        ListView {
            id: preview
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: dialog.scanning ? [] : dialog.rows
            keyNavigationEnabled: true
            activeFocusOnTab: true
            boundsBehavior: Flickable.StopAtBounds
            // Always shown when the list overflows: the headings can start below
            // the options (Urdu, small windows), and a hidden bar gave no hint.
            ScrollBar.vertical: ScrollBar {
                policy: preview.contentHeight > preview.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            }
            Accessible.role: Accessible.List
            Accessible.name: I18n.t("autoBookmarks.preview")
            Keys.onSpacePressed: if (currentIndex >= 0) dialog.toggle(dialog.rows[currentIndex])
            currentIndex: -1
            // Start at the top (options first) whenever the list changes.
            onModelChanged: { currentIndex = -1; Qt.callLater(() => preview.positionViewAtBeginning()) }

            header: ColumnLayout {
                width: preview.width
                spacing: Theme.sm
                // A ListView keeps its scroll offset when the header grows; the
                // options sit at the top, so show them whole again.
                onHeightChanged: if (preview.contentY < preview.originY + height) Qt.callLater(() => preview.positionViewAtBeginning())
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    text: I18n.t("autoBookmarks.intro")
                    color: Theme.muted
                    font.family: I18n.uiFamily
                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                }
                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.sm
                    horizontalAlignment: Text.AlignLeft
                    text: I18n.t("autoBookmarks.levels")
                    color: Theme.ink
                    font.family: I18n.uiFamily
                    font.weight: Font.DemiBold
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: Theme.md
                    Repeater {
                        model: 4
                        delegate: AtlasRadioButton {
                            required property int index
                            text: I18n.isolate(String(index + 1))
                            checked: dialog.levels === index + 1
                            onClicked: dialog.levels = index + 1
                            Accessible.name: I18n.t("autoBookmarks.levelsA11y", index + 1)
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.sm
                    horizontalAlignment: Text.AlignLeft
                    text: I18n.t("autoBookmarks.place")
                    color: Theme.ink
                    font.family: I18n.uiFamily
                    font.weight: Font.DemiBold
                }
                Repeater {
                    model: ["after", "before", "replace"]
                    delegate: AtlasRadioButton {
                        required property string modelData
                        Layout.fillWidth: true
                        text: I18n.t("autoBookmarks.place." + modelData)
                        checked: dialog.placement === modelData
                        onClicked: dialog.placement = modelData
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: dialog.placement === "replace"
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    text: I18n.t("autoBookmarks.replaceNote")
                    color: Theme.muted
                    font.family: I18n.uiFamily
                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                }
                // Progress while Atlas reads the book.
                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.md
                    visible: dialog.scanning
                    horizontalAlignment: Text.AlignLeft
                    text: I18n.t("autoBookmarks.checking", dialog.pagesDone, dialog.pageCount)
                    color: Theme.ink
                    font.family: I18n.uiFamily
                }
                ProgressBar {
                    Layout.fillWidth: true
                    visible: dialog.scanning
                    from: 0
                    to: Math.max(1, dialog.pageCount)
                    value: dialog.pagesDone
                }
                AtlasCheckBox {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.md
                    visible: !dialog.scanning
                    text: I18n.t("autoBookmarks.found", dialog.rows.length)
                    checked: dialog.rows.length > 0 && dialog.chosen === dialog.rows.length
                    enabled: dialog.rows.length > 0
                    onClicked: dialog.setAll(dialog.chosen !== dialog.rows.length)
                    Accessible.name: I18n.t("autoBookmarks.selectAll")
                }
                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.sm
                    visible: !dialog.scanning && dialog.rows.length === 0
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    text: I18n.t("autoBookmarks.none")
                    color: Theme.muted
                    font.family: I18n.uiFamily
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; visible: !dialog.scanning && dialog.rows.length > 0; color: Theme.chromeLine }
            }

            delegate: ItemDelegate {
                id: row
                required property int index
                required property int modelData
                readonly property var heading: dialog.headings[modelData]
                width: ListView.view.width
                height: Math.max(30, content.implicitHeight + 6)
                highlighted: ListView.isCurrentItem && preview.activeFocus
                onClicked: { preview.currentIndex = index; dialog.toggle(modelData) }
                Accessible.role: Accessible.CheckBox
                Accessible.checked: dialog.checks[modelData] === true
                Accessible.name: heading.title
                Accessible.description: I18n.t("autoBookmarks.rowA11y", heading.level + 1, dialog.pageLabel(heading.page + 1))
                    + (dialog.existing[modelData] === true ? ", " + I18n.t("autoBookmarks.already") : "")
                background: Rectangle {
                    color: row.highlighted ? Theme.accentSoft : row.hovered ? Theme.hover : "transparent"
                    border.width: row.visualFocus ? 2 : 0
                    border.color: Theme.focusRing
                }
                contentItem: RowLayout {
                    id: content
                    spacing: Theme.sm
                    Item { Layout.preferredWidth: row.heading.level * 18 }
                    Rectangle {   // check box drawn like AtlasCheckBox
                        implicitWidth: 16; implicitHeight: 16
                        radius: 2
                        readonly property bool on: dialog.checks[row.modelData] === true
                        color: on ? Theme.accent : Theme.raised
                        border.color: on ? Theme.accent : Theme.controlBorder
                        IconImage {
                            anchors.centerIn: parent
                            visible: parent.on
                            source: Theme.icon("checkmark")
                            sourceSize.width: 14; sourceSize.height: 14
                            color: Theme.accentText
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: row.heading.title
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        color: dialog.checks[row.modelData] ? Theme.ink : Theme.muted
                        font.family: I18n.contentFamily(text) || I18n.uiFamily
                        font.pixelSize: Math.round(13 * I18n.sizeScale)
                        font.weight: row.heading.level === 0 ? Font.DemiBold : Font.Normal
                    }
                    Text {
                        visible: dialog.existing[row.modelData] === true
                        text: I18n.t("autoBookmarks.already")
                        color: Theme.muted
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(11 * I18n.sizeScale)
                    }
                    Text {
                        text: dialog.pageLabel(row.heading.page + 1)
                        color: Theme.muted
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(11 * I18n.sizeScale)
                    }
                }
            }
            footer: Label {
                width: preview.width
                visible: !dialog.scanning && dialog.truncated
                height: visible ? implicitHeight + Theme.sm : 0
                topPadding: Theme.sm
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                text: I18n.t("autoBookmarks.truncated")
                color: Theme.muted
                font.family: I18n.uiFamily
                font.pixelSize: Math.round(12 * I18n.sizeScale)
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Theme.sm
            AtlasButton { text: I18n.t("prefs.cancel"); onClicked: dialog.close() }
            AtlasButton {
                kind: "primary"
                enabled: !dialog.scanning && dialog.chosen > 0
                text: I18n.t("autoBookmarks.create", dialog.chosen)
                onClicked: dialog.create()
            }
        }
    }
}
