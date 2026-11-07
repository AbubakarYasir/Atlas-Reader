import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Library: folders, favourites, recents and search (N3 behaviour, N4.3 design).
Item {
    id: library

    signal addFolderRequested()
    property bool gridMode: false
    property alias searchField: searchField

    // Book names can use any supported script regardless of interface language.
    // Reserve both title lines and metadata from actual font metrics, not a
    // fixed card height that lets the layout push actions outside the border.
    FontMetrics { id: latinTitleMetrics; font.family: I18n.systemFamily; font.pixelSize: 15; font.weight: Font.DemiBold }
    FontMetrics { id: arabicTitleMetrics; font.family: library.gridMode ? "Noto Naskh Arabic" : I18n.systemFamily; font.pixelSize: 15; font.weight: Font.DemiBold }
    FontMetrics { id: urduTitleMetrics; font.family: library.gridMode ? "Noto Nastaliq Urdu" : I18n.systemFamily; font.pixelSize: 15; font.weight: Font.DemiBold }
    FontMetrics { id: latinDetailMetrics; font.family: I18n.systemFamily; font.pixelSize: 12 }
    FontMetrics { id: arabicDetailMetrics; font.family: library.gridMode ? "Noto Naskh Arabic" : I18n.systemFamily; font.pixelSize: 12 }
    FontMetrics { id: urduDetailMetrics; font.family: library.gridMode ? "Noto Nastaliq Urdu" : I18n.systemFamily; font.pixelSize: 12 }
    readonly property real gridTitleLineHeight: Math.ceil(Math.max(latinTitleMetrics.height, arabicTitleMetrics.height, urduTitleMetrics.height))
    readonly property real gridDetailHeight: Math.ceil(Math.max(latinDetailMetrics.height, arabicDetailMetrics.height, urduDetailMetrics.height))
    readonly property real gridCardHeight: Theme.libraryCoverHeight + 2 * gridTitleLineHeight
        + gridDetailHeight + Theme.controlHeight + 4 * Theme.sm + 2 * Theme.md + 2 * Theme.xs

    function statusText(value) {
        if (value === "scanning") return I18n.t("library.status.scanning")
        if (value === "complete") return I18n.t("library.status.complete", libraryController.lastAddedCount)
        if (value === "completeWithErrors") return I18n.t("library.status.completeWithErrors")
        if (value === "noRoots") return I18n.t("library.status.noRoots")
        if (value === "error") return I18n.t("library.status.error")
        return I18n.t("library.status.ready")
    }

    function availabilityText(value) {
        const known = ["available", "locked", "offline", "missing", "unreadable", "unsupported", "readOnly"]
        return I18n.t("availability." + (known.indexOf(value) >= 0 ? value : "unavailable"))
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- sidebar
        Rectangle {
            Layout.preferredWidth: library.width < 900 ? 200 : 240
            Layout.fillHeight: true
            color: Theme.panel
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.md
                spacing: Theme.xs

                Label {
                    text: I18n.t("library.section.library")
                    color: Theme.muted
                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                    font.weight: Font.DemiBold
                    Layout.leftMargin: Theme.sm
                    Layout.bottomMargin: Theme.xs
                }
                SidebarItem {
                    iconName: "library"
                    text: I18n.t("library.allBooks")
                    selected: libraryController.viewMode === 0 && libraryController.selectedRootId === ""
                    onClicked: { libraryController.selectedRootId = ""; libraryController.viewMode = 0 }
                }
                SidebarItem {
                    iconName: "star"
                    text: I18n.t("library.favorites")
                    selected: libraryController.viewMode === 1
                    onClicked: libraryController.viewMode = 1
                }
                SidebarItem {
                    iconName: "history"
                    text: I18n.t("library.recent")
                    selected: libraryController.viewMode === 2
                    onClicked: libraryController.viewMode = 2
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line; Layout.topMargin: Theme.sm; Layout.bottomMargin: Theme.sm }
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: I18n.t("library.section.folders")
                        color: Theme.muted
                        font.pixelSize: Math.round(12 * I18n.sizeScale)
                        font.weight: Font.DemiBold
                        Layout.leftMargin: Theme.sm
                    }
                    AtlasIconButton {
                        iconName: "folder_add"
                        text: I18n.t("library.addFolder")
                        shortcutHint: "Ctrl+Shift+O"
                        onClicked: library.addFolderRequested()
                    }
                }
                Repeater {
                    model: libraryController.roots
                    delegate: SidebarItem {
                        required property var modelData
                        iconName: modelData.availability === "available" ? "folder_open" : "warning"
                        text: modelData.name
                        detail: modelData.availability === "available" ? "" : I18n.t("library.folderOffline")
                        selected: libraryController.selectedRootId === modelData.id
                        Accessible.description: modelData.path
                        onClicked: libraryController.selectedRootId = modelData.id
                    }
                }
                Item { Layout.fillHeight: true }
                AtlasButton {
                    Layout.fillWidth: true
                    visible: libraryController.pendingCount > 0
                    iconName: "warning"
                    text: I18n.t("library.reviewChanges", libraryController.pendingCount)
                    onClicked: reviewPopup.open()
                }
                Label {
                    Layout.fillWidth: true
                    text: I18n.ltr(atlasVersion)
                    color: Theme.muted
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // ---- main
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.xl
            spacing: Theme.lg

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sm
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: libraryController.viewMode === 1 ? I18n.t("library.favorites")
                            : libraryController.viewMode === 2 ? I18n.t("library.recent")
                            : libraryController.selectedRootName !== "" ? libraryController.selectedRootName
                            : I18n.t("library.title")
                        color: Theme.ink
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(26 * I18n.sizeScale)
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: I18n.t("library.subtitle")
                        color: Theme.muted
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(14 * I18n.sizeScale)
                        elide: Text.ElideRight
                    }
                }
                AtlasButton {
                    iconName: "arrow_sync"
                    text: I18n.t("library.rescan")
                    enabled: !libraryController.scanning && libraryController.rootCount > 0
                    Accessible.description: "F5"
                    onClicked: libraryController.rescan()
                }
                AtlasButton {
                    kind: "primary"
                    iconName: "folder_add"
                    text: I18n.t("library.addFolder")
                    Accessible.description: "Ctrl+Shift+O"
                    onClicked: library.addFolderRequested()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sm
                AtlasTextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: I18n.t("library.searchPlaceholder")
                    Accessible.name: I18n.t("library.search")
                    selectByMouse: true
                    onTextEdited: searchDelay.restart()
                    Keys.onEscapePressed: { text = ""; libraryController.query = "" }
                    Timer { id: searchDelay; interval: 180; onTriggered: libraryController.query = searchField.text }
                }
                AtlasIconButton {
                    iconName: "list"
                    text: I18n.t("library.listView")
                    checkable: true
                    checked: !library.gridMode
                    onClicked: library.gridMode = false
                }
                AtlasIconButton {
                    iconName: "grid"
                    text: I18n.t("library.gridView")
                    checkable: true
                    checked: library.gridMode
                    onClicked: library.gridMode = true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                visible: libraryController.statusKey !== "ready"
                implicitHeight: Theme.controlHeight + Theme.sm
                radius: Theme.radiusControl
                color: libraryController.statusKey === "error" ? Theme.dangerSoft : Theme.accentSoft
                border.color: libraryController.statusKey === "error" ? Theme.dangerBorder : Qt.alpha(Theme.accent, 0.5)
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.md
                    anchors.rightMargin: Theme.md
                    BusyIndicator { running: libraryController.scanning; visible: running; implicitWidth: 20; implicitHeight: 20 }
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: library.statusText(libraryController.statusKey)
                        color: libraryController.statusKey === "error" ? Theme.danger : Theme.ink
                        elide: Text.ElideRight
                    }
                    AtlasButton {
                        visible: libraryController.pendingCount > 0
                        kind: "quiet"
                        text: I18n.t("library.review")
                        onClicked: reviewPopup.open()
                    }
                }
            }

            GridView {
                id: bookView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: libraryController
                cellWidth: library.gridMode ? Math.max(220, width / Math.max(1, Math.floor(width / 260))) : width
                cellHeight: library.gridMode ? library.gridCardHeight : 84 + 2 * Theme.scriptExtra
                keyNavigationEnabled: true
                activeFocusOnTab: true
                ScrollBar.vertical: ScrollBar {}

                delegate: Item {
                    id: bookCell
                    required property int index
                    required property string documentId
                    required property string bookTitle
                    required property string bookAuthor
                    required property string sourcePath
                    required property string fileName
                    required property string fileStem
                    required property string fileExtension
                    required property string availability
                    required property bool favorite
                    readonly property bool canOpen: availability === "available" || availability === "readOnly"
                    readonly property bool hasDistinctTitle: bookTitle.length > 0
                        && bookTitle.toLowerCase() !== fileStem.toLowerCase()
                        && bookTitle.toLowerCase() !== fileName.toLowerCase()
                    readonly property string displayTitle: fileStem || fileName
                    readonly property string detailLine: [hasDistinctTitle ? bookTitle : "", bookAuthor]
                        .filter(function(part) { return part.length > 0 }).join("  ·  ")
                    readonly property bool showBadge: !canOpen || availability === "readOnly"
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight
                    Accessible.role: Accessible.ListItem
                    Accessible.name: displayTitle

                    Rectangle {
                        id: card
                        anchors.fill: parent
                        anchors.margins: Theme.xs
                        radius: Theme.radiusSurface
                        color: hoverArea.hovered ? Theme.hover : Theme.raised
                        border.color: bookView.activeFocus && bookCell.GridView.isCurrentItem ? Theme.focusRing : Theme.line
                        border.width: bookView.activeFocus && bookCell.GridView.isCurrentItem ? 2 : 1
                        Behavior on color { ColorAnimation { duration: Theme.fast } }
                        HoverHandler { id: hoverArea }
                        TapHandler {
                            onTapped: bookView.currentIndex = bookCell.index
                            onDoubleTapped: if (bookCell.canOpen) libraryController.openInReader(bookCell.documentId)
                        }

                        // ---- list row: cover · text · actions, vertically centred
                        RowLayout {
                            visible: !library.gridMode
                            anchors.fill: parent
                            anchors.leftMargin: Theme.md
                            anchors.rightMargin: Theme.md
                            spacing: Theme.md

                            Rectangle {
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: 56
                                radius: Theme.radiusSmall
                                color: bookCell.canOpen ? Theme.accentSoft : Theme.warningSoft
                                border.color: Theme.line
                                Label {
                                    anchors.centerIn: parent
                                    text: (bookCell.fileExtension || "FILE").toUpperCase()
                                    color: bookCell.canOpen ? Theme.accent : Theme.warning
                                    font.family: I18n.systemFamily
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 0
                                Label {
                                    Layout.fillWidth: true
                                    text: bookCell.displayTitle
                                    color: Theme.ink
                                    font.family: I18n.contentFamily(text)
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                    horizontalAlignment: Text.AlignLeft
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: bookCell.detailLine.length > 0
                                    text: bookCell.detailLine
                                    color: Theme.muted
                                    font.family: I18n.contentFamily(text)
                                    font.pixelSize: 13
                                    horizontalAlignment: Text.AlignLeft
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: I18n.ltr(bookCell.sourcePath)
                                    color: Theme.muted
                                    font.family: I18n.systemFamily
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignLeft
                                    elide: Text.ElideMiddle
                                }
                            }

                            Rectangle {
                                visible: bookCell.showBadge
                                radius: Theme.radiusSmall
                                color: Theme.warningSoft
                                implicitWidth: listBadge.implicitWidth + Theme.md
                                implicitHeight: listBadge.implicitHeight + Theme.xs
                                Label {
                                    id: listBadge
                                    anchors.centerIn: parent
                                    text: library.availabilityText(bookCell.availability)
                                    color: Theme.warning
                                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                                    font.weight: Font.DemiBold
                                }
                            }
                            AtlasIconButton {
                                iconName: bookCell.favorite ? "star_filled" : "star"
                                checkable: true
                                checked: bookCell.favorite
                                text: bookCell.favorite ? I18n.t("library.unfavorite") : I18n.t("library.favorite")
                                onClicked: libraryController.setFavorite(bookCell.documentId, !bookCell.favorite)
                            }
                            AtlasButton {
                                text: I18n.t("action.open")
                                enabled: bookCell.canOpen
                                Accessible.description: I18n.t("library.openDescription")
                                onClicked: libraryController.openInReader(bookCell.documentId)
                            }
                        }

                        // ---- grid card: cover on top, text, actions
                        ColumnLayout {
                            visible: library.gridMode
                            anchors.fill: parent
                            anchors.margins: Theme.md
                            spacing: Theme.sm

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: Theme.libraryCoverHeight
                                Layout.minimumHeight: Theme.libraryCoverHeight
                                Layout.maximumHeight: Theme.libraryCoverHeight
                                radius: Theme.radiusControl
                                color: bookCell.canOpen ? Theme.accentSoft : Theme.warningSoft
                                IconImage {
                                    anchors.centerIn: parent
                                    source: Theme.icon("document")
                                    sourceSize.width: 36
                                    sourceSize.height: 36
                                    color: bookCell.canOpen ? Theme.accent : Theme.warning
                                }
                                Label {
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.margins: Theme.sm
                                    text: (bookCell.fileExtension || "FILE").toUpperCase()
                                    color: bookCell.canOpen ? Theme.accent : Theme.warning
                                    font.family: I18n.systemFamily
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                text: bookCell.displayTitle
                                color: Theme.ink
                                font.family: I18n.contentFamily(text)
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignLeft
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true
                                text: bookCell.detailLine.length > 0 ? bookCell.detailLine : I18n.ltr(bookCell.sourcePath)
                                color: Theme.muted
                                font.family: bookCell.detailLine.length > 0 ? I18n.contentFamily(text) : I18n.systemFamily
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignLeft
                                elide: bookCell.detailLine.length > 0 ? Text.ElideRight : Text.ElideMiddle
                            }
                            Item { Layout.fillHeight: true }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.xs
                                Rectangle {
                                    visible: bookCell.showBadge
                                    radius: Theme.radiusSmall
                                    color: Theme.warningSoft
                                    implicitWidth: gridBadge.implicitWidth + Theme.md
                                    implicitHeight: gridBadge.implicitHeight + Theme.xs
                                    Label {
                                        id: gridBadge
                                        anchors.centerIn: parent
                                        text: library.availabilityText(bookCell.availability)
                                        color: Theme.warning
                                        font.pixelSize: Math.round(12 * I18n.sizeScale)
                                        font.weight: Font.DemiBold
                                    }
                                }
                                Item { Layout.fillWidth: true }
                                AtlasIconButton {
                                    iconName: bookCell.favorite ? "star_filled" : "star"
                                    checkable: true
                                    checked: bookCell.favorite
                                    text: bookCell.favorite ? I18n.t("library.unfavorite") : I18n.t("library.favorite")
                                    onClicked: libraryController.setFavorite(bookCell.documentId, !bookCell.favorite)
                                }
                                AtlasButton {
                                    text: I18n.t("action.open")
                                    enabled: bookCell.canOpen
                                    Accessible.description: I18n.t("library.openDescription")
                                    onClicked: libraryController.openInReader(bookCell.documentId)
                                }
                            }
                        }
                    }
                }

                Keys.onReturnPressed: {
                    const item = bookView.currentItem
                    if (item && item.canOpen) libraryController.openInReader(item.documentId)
                }

                // ---- empty state
                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(460, parent.width - 40)
                    visible: bookView.count === 0
                    spacing: Theme.md
                    IconImage {
                        Layout.alignment: Qt.AlignHCenter
                        source: Theme.icon(searchField.text.length > 0 ? "search" : "library")
                        sourceSize.width: 40
                        sourceSize.height: 40
                        color: Theme.muted
                    }
                    Label {
                        Layout.fillWidth: true
                        text: searchField.text.length > 0 ? I18n.t("library.empty.noMatch")
                            : libraryController.rootCount === 0 ? I18n.t("library.empty.noFolders")
                            : I18n.t("library.empty.noBooks")
                        color: Theme.muted
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(15 * I18n.sizeScale)
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                    AtlasButton {
                        Layout.alignment: Qt.AlignHCenter
                        visible: libraryController.rootCount === 0 && searchField.text.length === 0
                        kind: "primary"
                        iconName: "folder_add"
                        text: I18n.t("library.addFolder")
                        onClicked: library.addFolderRequested()
                    }
                }
            }
        }
    }

    // ---- review of moves/replacements (N3 identity safety)
    Popup {
        id: reviewPopup
        anchors.centerIn: parent
        width: Math.min(700, library.width - 48)
        height: Math.min(560, library.height - 72)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: Theme.panel; radius: Theme.radiusSurface; border.color: Theme.line }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.xl
            spacing: Theme.md

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    text: I18n.t("review.title")
                    color: Theme.ink
                    font.pixelSize: Math.round(20 * I18n.sizeScale)
                    font.weight: Font.DemiBold
                }
                AtlasIconButton { iconName: "dismiss"; text: I18n.t("action.close"); onClicked: reviewPopup.close() }
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: I18n.t("review.explanation")
                color: Theme.muted
                wrapMode: Text.WordWrap
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: Theme.sm
                model: libraryController.pendingProposals
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: proposalColumn.implicitHeight + Theme.xl
                    radius: Theme.radiusControl
                    color: Theme.raised
                    border.color: Theme.line
                    ColumnLayout {
                        id: proposalColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.md
                        spacing: Theme.xs
                        Label {
                            text: modelData.canApply ? I18n.t("review.verified") : I18n.t("review.ambiguous")
                            color: modelData.canApply ? Theme.accent : Theme.warning
                            font.weight: Font.DemiBold
                        }
                        Label { Layout.fillWidth: true; text: I18n.t("review.from", I18n.isolate(modelData.beforePath)); color: Theme.muted; elide: Text.ElideMiddle }
                        Label { Layout.fillWidth: true; text: I18n.t("review.to", I18n.isolate(modelData.candidatePath)); color: Theme.ink; elide: Text.ElideMiddle }
                        RowLayout {
                            Layout.alignment: Qt.AlignRight
                            AtlasButton { text: I18n.t("review.dismiss"); onClicked: libraryController.dismissProposal(modelData.id) }
                            AtlasButton {
                                visible: modelData.canApply
                                kind: "primary"
                                text: I18n.t("review.apply")
                                onClicked: libraryController.applyProposal(modelData.id)
                            }
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: libraryController.pendingCount === 0
                    text: I18n.t("review.none")
                    color: Theme.muted
                }
            }
        }
    }
}
