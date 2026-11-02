import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Search panel (Foxit Advanced Search, Ctrl+Shift+F; N4.5): docked on the
// trailing side. Results stream in page by page with their surrounding text;
// Arabic and Urdu matching ignores harakat, tatweel and letter variants.
Rectangle {
    id: panel

    required property var view          // ReaderViewport
    signal closeRequested()

    function focusField(text) {
        if (text && text.length > 0) field.text = text
        field.forceActiveFocus()
        field.selectAll()
    }
    function run() {
        view.find(field.text, preferences.searchMatchCase, preferences.searchWholeWords)
    }

    color: Theme.panel
    Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.chromeLine }

    Connections {
        target: preferences
        function onSearchOptionsChanged() { if (panel.visible && field.text.trim().length > 0) panel.run() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 1
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
                text: I18n.t("panel.search")
                color: Theme.ink
                font.pixelSize: Math.round(13 * I18n.sizeScale)
            }
            AtlasIconButton {
                implicitWidth: 26; implicitHeight: 26; iconSize: 14; cornerRadius: 2
                iconName: "dismiss"
                text: I18n.t("action.close")
                onClicked: panel.closeRequested()
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.spacing }

        AtlasTextField {
            id: field
            Layout.fillWidth: true
            Layout.margins: Theme.sm
            placeholderText: I18n.t("find.placeholder")
            selectByMouse: true
            font.family: I18n.contentFamily(text) || I18n.uiFamily
            Accessible.name: I18n.t("panel.search")
            onAccepted: panel.run()
            Keys.onEscapePressed: panel.closeRequested()
        }
        ColumnLayout {
            Layout.leftMargin: Theme.sm
            Layout.rightMargin: Theme.sm
            spacing: 0
            AtlasCheckBox {
                text: I18n.t("find.wholeWords")
                checked: preferences.searchWholeWords
                onToggled: preferences.searchWholeWords = checked
                font.pixelSize: Math.round(12 * I18n.sizeScale)
                padding: 2
            }
            AtlasCheckBox {
                text: I18n.t("find.matchCase")
                checked: preferences.searchMatchCase
                onToggled: preferences.searchMatchCase = checked
                font.pixelSize: Math.round(12 * I18n.sizeScale)
                padding: 2
            }
        }
        AtlasButton {
            Layout.alignment: Qt.AlignRight
            Layout.margins: Theme.sm
            kind: "primary"
            iconName: "search"
            text: I18n.t("find.search")
            enabled: field.text.trim().length > 0
            onClicked: panel.run()
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.md
            Layout.rightMargin: Theme.md
            visible: !panel.view.hasText && panel.view.textReady
            text: I18n.t("find.noText")
            color: Theme.warning
            wrapMode: Text.WordWrap
            font.pixelSize: Math.round(12 * I18n.sizeScale)
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.md
            Layout.rightMargin: Theme.md
            Layout.bottomMargin: Theme.xs
            visible: panel.view.searchQuery.length > 0
            text: panel.view.searching
                ? I18n.t("find.progress", panel.view.searchHits.length, Math.round(panel.view.searchProgress * 100))
                : panel.view.searchHits.length === 0 ? I18n.t("find.noResults")
                : I18n.t("find.total", panel.view.searchHits.length)
            color: Theme.muted
            wrapMode: Text.WordWrap
            font.pixelSize: Math.round(12 * I18n.sizeScale)
        }
        ProgressBar {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.md
            Layout.rightMargin: Theme.md
            visible: panel.view.searching
            value: panel.view.searchProgress
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.spacing }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            activeFocusOnTab: true
            keyNavigationEnabled: true
            model: panel.view.searchHits
            currentIndex: panel.view.searchIndex
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
            ScrollBar.vertical: ScrollBar {}
            Accessible.role: Accessible.List
            Accessible.name: I18n.t("find.results")
            Keys.onReturnPressed: panel.view.showSearchHit(currentIndex)

            delegate: AbstractButton {
                id: hitRow
                required property int index
                required property var modelData
                width: ListView.view.width
                implicitHeight: hitText.implicitHeight + 10
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                Accessible.role: Accessible.ListItem
                Accessible.name: modelData.before + " " + modelData.match + " " + modelData.after
                Accessible.selected: index === panel.view.searchIndex
                onClicked: panel.view.showSearchHit(index)
                background: Rectangle {
                    color: hitRow.index === panel.view.searchIndex ? Theme.accentSoft : hitRow.hovered ? Theme.hover : "transparent"
                }
                Text {
                    id: pageNumber
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.sm
                    anchors.top: parent.top
                    anchors.topMargin: 5
                    text: I18n.t("find.pageShort", hitRow.modelData.page + 1)
                    color: Theme.muted
                    font.pixelSize: 11
                }
                contentItem: Text {
                    id: hitText
                    // the page number sits on the trailing side in either direction
                    leftPadding: hitRow.mirrored ? pageNumber.implicitWidth + Theme.sm * 2 : Theme.md
                    rightPadding: hitRow.mirrored ? Theme.md : pageNumber.implicitWidth + Theme.sm * 2
                    topPadding: 5
                    bottomPadding: 5
                    textFormat: Text.StyledText
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    color: Theme.ink
                    font.family: I18n.contentFamily(hitRow.modelData.match)
                    font.pixelSize: 12
                    text: panel.escapeHtml(hitRow.modelData.before) + " <b>" + panel.escapeHtml(hitRow.modelData.match) + "</b> "
                        + panel.escapeHtml(hitRow.modelData.after)
                }
            }
        }
    }

    function escapeHtml(s) {
        return String(s || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }
}
