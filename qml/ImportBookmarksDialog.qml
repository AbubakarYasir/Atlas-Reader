import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Import Bookmarks (N5.5): what the file holds, a warning when it came from
// another book, and where the bookmarks go. Nothing changes until Import; the
// whole import is one Undo step.
Popup {
    id: dialog

    property var info: ({})             // BookmarkModel.readImport()
    property string selectedTitle: ""
    property bool hasBookmarks: false
    signal accepted(string mode)

    function ask(preview) {
        info = preview
        atEnd.checked = true
        open()
    }

    modal: true
    focus: true
    parent: Overlay.overlay
    x: Math.round(((parent ? parent.width : 800) - width) / 2)
    y: Math.round(((parent ? parent.height : 600) - height) / 3)
    width: Math.min(480, (parent ? parent.width : 800) - 40)
    padding: Theme.xl
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: Theme.raised; radius: Theme.radiusSurface; border.color: Theme.chromeLine }

    contentItem: ColumnLayout {
        spacing: Theme.sm
        Accessible.role: Accessible.Dialog
        Accessible.name: I18n.t("exchange.importTitle")

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            text: I18n.t("exchange.importTitle")
            color: Theme.ink
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(16 * I18n.sizeScale)
            font.weight: Font.DemiBold
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: I18n.t("exchange.importSummary", dialog.info.count || 0)
                  + ((dialog.info.topTitles || []).length > 0 ? " " + I18n.t("exchange.startsWith", dialog.info.topTitles.join(I18n.t("exchange.listSeparator"))) : "")
            color: Theme.ink
            font.family: I18n.uiFamily
        }
        Label { // the file came from another book: pages may not match
            Layout.fillWidth: true
            visible: dialog.info.sameBook === false
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: I18n.t("exchange.otherBook", dialog.info.fileName || "")
            color: Theme.warning
            font.family: I18n.uiFamily
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: Theme.xs
            horizontalAlignment: Text.AlignLeft
            text: I18n.t("exchange.place")
            color: Theme.muted
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(12 * I18n.sizeScale)
            font.weight: Font.DemiBold
        }
        ButtonGroup { id: modes }
        AtlasRadioButton { id: atEnd; Layout.fillWidth: true; ButtonGroup.group: modes; text: I18n.t("exchange.modeEnd") }
        AtlasRadioButton {
            id: under
            Layout.fillWidth: true
            ButtonGroup.group: modes
            visible: dialog.selectedTitle.length > 0
            text: I18n.t("exchange.modeUnder", dialog.selectedTitle)
        }
        AtlasRadioButton { id: merge; Layout.fillWidth: true; ButtonGroup.group: modes; visible: dialog.hasBookmarks; text: I18n.t("exchange.modeMerge") }
        AtlasRadioButton { id: replace; Layout.fillWidth: true; ButtonGroup.group: modes; visible: dialog.hasBookmarks; text: I18n.t("exchange.modeReplace") }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: I18n.t("exchange.importSafety")
            color: Theme.muted
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(12 * I18n.sizeScale)
        }
        RowLayout {
            Layout.topMargin: Theme.sm
            Layout.alignment: Qt.AlignRight
            spacing: Theme.sm
            AtlasButton { text: I18n.t("prefs.cancel"); onClicked: dialog.close() }
            AtlasButton {
                kind: "primary"
                iconName: "arrow_import"
                text: I18n.t("exchange.importAction")
                onClicked: {
                    dialog.close()
                    dialog.accepted(under.checked ? "under" : merge.checked ? "merge" : replace.checked ? "replace" : "end")
                }
            }
        }
    }
}
