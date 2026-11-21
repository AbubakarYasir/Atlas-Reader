import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Signatures panel (Foxit navigation pane → Digital Signatures, N5.3):
// status only. Lists the PDF's signature fields from the capability check —
// signed or empty, and on which page — and says when the document is
// certified. Atlas does not validate signatures; the panel says so.
Rectangle {
    id: panel

    required property var view          // ReaderViewport
    property var capability: null       // ReaderTab.capability (N5.1 check)

    readonly property bool checked: capability !== null && capability !== undefined && capability.checked === true
    readonly property bool inspected: checked && capability.inspected === true
    readonly property var fields: inspected && capability.signatures ? capability.signatures : []
    readonly property int signedCount: fields.filter(f => f.signed).length

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
            text: I18n.t("panel.signatures")
            color: Theme.ink
            font.pixelSize: Math.round(13 * I18n.sizeScale)
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.spacing }

        // Certified: the author locked the document against changes.
        Rectangle {
            Layout.fillWidth: true
            Layout.margins: Theme.sm
            visible: panel.inspected && panel.capability.certified === true
            implicitHeight: certifiedRow.implicitHeight + Theme.sm * 2
            radius: Theme.radiusSmall
            color: Theme.warningSoft
            RowLayout {
                id: certifiedRow
                anchors.fill: parent
                anchors.margins: Theme.sm
                spacing: Theme.sm
                IconImage {
                    Layout.alignment: Qt.AlignTop
                    source: Theme.icon("lock_closed")
                    sourceSize.width: 16; sourceSize.height: 16
                    color: Theme.ink
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignLeft
                    text: I18n.t("signatures.certified")
                    color: Theme.ink
                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                }
            }
        }

        Label {   // summary, or why there is nothing to list
            Layout.fillWidth: true
            Layout.leftMargin: Theme.md
            Layout.rightMargin: Theme.md
            Layout.topMargin: Theme.sm
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            color: panel.fields.length > 0 ? Theme.ink : Theme.muted
            font.pixelSize: Math.round(12 * I18n.sizeScale)
            text: !panel.checked ? I18n.t("signatures.checking")
                : !panel.inspected ? I18n.t("signatures.notInspected")
                : panel.fields.length === 0 ? I18n.t("signatures.none")
                : I18n.t("signatures.summary", panel.fields.length, panel.signedCount)
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.sm
            clip: true
            model: panel.fields
            keyNavigationEnabled: true
            activeFocusOnTab: true
            ScrollBar.vertical: ScrollBar {}
            Accessible.role: Accessible.List
            Accessible.name: I18n.t("panel.signatures")
            Keys.onReturnPressed: if (currentItem) currentItem.clicked()

            delegate: ItemDelegate {
                id: row
                required property int index
                required property var modelData
                width: ListView.view.width
                height: Math.max(44, content.implicitHeight + 12)
                highlighted: ListView.isCurrentItem && list.activeFocus
                enabled: modelData.page > 0
                Accessible.role: Accessible.ListItem
                Accessible.name: modelData.name + ", " + stateLabel.text + ", " + pageLabel.text
                onClicked: { list.currentIndex = index; if (panel.view) panel.view.goToPage(modelData.page) }
                background: Rectangle {
                    color: row.highlighted ? Theme.accentSoft : row.hovered ? Theme.hover : "transparent"
                    border.width: row.visualFocus ? 2 : 0
                    border.color: Theme.focusRing
                }
                contentItem: RowLayout {
                    id: content
                    spacing: Theme.sm
                    IconImage {
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 2
                        source: Theme.icon("signature")
                        sourceSize.width: 16; sourceSize.height: 16
                        color: row.modelData.signed ? Theme.ink : Theme.muted
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: row.modelData.name.length > 0 ? row.modelData.name : I18n.t("signatures.unnamed")
                            textFormat: Text.PlainText
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            color: Theme.ink
                            font.family: I18n.contentFamily(text) || I18n.uiFamily
                            font.pixelSize: Math.round(13 * I18n.sizeScale)
                        }
                        Text {
                            id: stateLabel
                            Layout.fillWidth: true
                            text: I18n.t(row.modelData.signed ? "signatures.signed" : "signatures.empty")
                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.WordWrap
                            color: Theme.muted
                            font.family: I18n.uiFamily
                            font.pixelSize: Math.round(11 * I18n.sizeScale)
                        }
                    }
                    Text {
                        id: pageLabel
                        Layout.alignment: Qt.AlignTop
                        visible: row.modelData.page > 0
                        text: I18n.t("signatures.page", row.modelData.page)
                        color: Theme.muted
                        font.family: I18n.uiFamily
                        font.pixelSize: Math.round(11 * I18n.sizeScale)
                    }
                }
            }
        }

        // Status only: no claim about validity.
        Label {
            Layout.fillWidth: true
            Layout.margins: Theme.md
            visible: panel.inspected
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            text: I18n.t("signatures.notValidated")
            color: Theme.muted
            font.pixelSize: Math.round(11 * I18n.sizeScale)
        }
    }
}
