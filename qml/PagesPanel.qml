import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Pages panel (Foxit navigation pane → Pages, N4.4): page thumbnails rendered
// in the background; the current page is outlined; click or Enter jumps there.
Rectangle {
    id: panel

    required property var view          // ReaderViewport
    required property var sessionId
    property int revision: 0

    color: Theme.panel

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.md
            Layout.topMargin: Theme.sm
            Layout.bottomMargin: Theme.sm
            horizontalAlignment: Text.AlignLeft
            text: I18n.t("panel.pages")
            color: Theme.ink
            font.pixelSize: Math.round(13 * I18n.sizeScale)
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.spacing }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            keyNavigationEnabled: true
            activeFocusOnTab: true
            model: panel.view ? panel.view.pageCount : 0
            readonly property int thumbWidth: 104
            cellWidth: Math.max(thumbWidth + 24, width / Math.max(1, Math.floor(width / (thumbWidth + 24))))
            cellHeight: thumbWidth * 1.42 + 34
            currentIndex: panel.view ? panel.view.currentPage - 1 : -1
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, GridView.Contain)
            ScrollBar.vertical: ScrollBar {}
            Accessible.role: Accessible.List
            Accessible.name: I18n.t("panel.pages")
            Keys.onReturnPressed: if (panel.view) panel.view.goToPage(currentIndex + 1)

            delegate: Item {
                id: cell
                required property int index
                readonly property bool current: panel.view && panel.view.currentPage === index + 1
                readonly property size points: panel.view ? panel.view.pageSizePoints(index + 1) : Qt.size(612, 792)
                width: GridView.view.cellWidth
                height: GridView.view.cellHeight
                Accessible.role: Accessible.ListItem
                Accessible.name: I18n.t("panel.pageA11y", index + 1)
                Accessible.selected: current

                Rectangle {
                    id: frame
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 8
                    width: grid.thumbWidth + 6
                    height: Math.min(grid.thumbWidth * 1.42, grid.thumbWidth * (cell.points.height / Math.max(1, cell.points.width))) + 6
                    color: "transparent"
                    border.width: cell.current ? 2 : (hover.hovered ? 1 : 0)
                    border.color: cell.current ? Theme.accent : Theme.buttonBorder
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 3
                        color: Theme.readingPaper[preferences.readingColors]
                        border.color: Theme.line
                        Image {
                            anchors.fill: parent
                            anchors.margins: 1
                            asynchronous: true
                            cache: true
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: grid.thumbWidth * 2
                            source: panel.sessionId > 0 && cell.GridView.isCurrentItem !== undefined
                                ? "image://atlasthumb/" + panel.sessionId + "/" + panel.revision + "/" + (cell.index + 1) + "/" + preferences.readingColors
                                : ""
                        }
                    }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: frame.bottom
                    anchors.topMargin: 3
                    text: cell.index + 1
                    color: cell.current ? Theme.accent : Theme.muted
                    font.pixelSize: 12
                }
                HoverHandler { id: hover }
                TapHandler { onTapped: if (panel.view) panel.view.goToPage(cell.index + 1) }
            }
        }
    }
}
