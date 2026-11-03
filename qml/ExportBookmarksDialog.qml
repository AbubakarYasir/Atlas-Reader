import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Export Bookmarks (N5.5): pick the format and which bookmarks, then the file.
// The Atlas bookmarks file is complete and can be imported again; Markdown and
// CSV are for reading and spreadsheets.
Popup {
    id: dialog

    property string selectedTitle: ""   // the selected bookmark, for "this and below"
    property bool hasLocal: false       // any bookmarks made or changed in Atlas
    signal accepted(string format, string scope)

    function ask() {
        json.checked = true
        all.checked = true
        open()
    }

    modal: true
    focus: true
    parent: Overlay.overlay
    x: Math.round(((parent ? parent.width : 800) - width) / 2)
    y: Math.round(((parent ? parent.height : 600) - height) / 3)
    width: Math.min(480, (parent ? parent.width : 800) - 40)
    height: Math.min(body.implicitHeight + footer.implicitHeight + Theme.md + padding * 2,
                     (parent ? parent.height : 600) - Theme.xl * 2)
    padding: Theme.xl
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: Theme.raised; radius: Theme.radiusSurface; border.color: Theme.chromeLine }

    component Heading: Label {
        Layout.fillWidth: true
        Layout.topMargin: Theme.xs
        horizontalAlignment: Text.AlignLeft
        color: Theme.muted
        font.family: I18n.uiFamily
        font.pixelSize: Math.round(12 * I18n.sizeScale)
        font.weight: Font.DemiBold
    }

    contentItem: ColumnLayout {
        spacing: Theme.sm
        Accessible.role: Accessible.Dialog
        Accessible.name: I18n.t("exchange.exportTitle")

        ScrollView {
            id: bodyScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: body.implicitHeight
            contentWidth: availableWidth
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ColumnLayout {
                id: body
                width: bodyScroll.availableWidth
                spacing: Theme.sm
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    text: I18n.t("exchange.exportTitle")
                    color: Theme.ink
                    font.family: I18n.uiFamily
                    font.pixelSize: Math.round(16 * I18n.sizeScale)
                    font.weight: Font.DemiBold
                }
                Heading { text: I18n.t("exchange.format") }
                ButtonGroup { id: formats }
                AtlasRadioButton { id: json; Layout.fillWidth: true; ButtonGroup.group: formats; text: I18n.t("exchange.formatJson") }
                AtlasRadioButton { id: markdown; Layout.fillWidth: true; ButtonGroup.group: formats; text: I18n.t("exchange.formatMarkdown") }
                AtlasRadioButton { id: csv; Layout.fillWidth: true; ButtonGroup.group: formats; text: I18n.t("exchange.formatCsv") }
                Heading { text: I18n.t("exchange.scope") }
                ButtonGroup { id: scopes }
                AtlasRadioButton { id: all; Layout.fillWidth: true; ButtonGroup.group: scopes; text: I18n.t("exchange.scopeAll") }
                AtlasRadioButton {
                    id: subtree
                    Layout.fillWidth: true
                    ButtonGroup.group: scopes
                    visible: dialog.selectedTitle.length > 0
                    text: I18n.t("exchange.scopeSubtree", dialog.selectedTitle)
                }
                AtlasRadioButton {
                    id: local
                    Layout.fillWidth: true
                    ButtonGroup.group: scopes
                    enabled: dialog.hasLocal
                    text: I18n.t("exchange.scopeLocal")
                }
            }
        }
        RowLayout {
            id: footer
            Layout.topMargin: Theme.sm
            Layout.alignment: Qt.AlignRight
            spacing: Theme.sm
            AtlasButton { text: I18n.t("prefs.cancel"); onClicked: dialog.close() }
            AtlasButton {
                kind: "primary"
                iconName: "arrow_export"
                text: I18n.t("exchange.exportAction")
                onClicked: {
                    dialog.close()
                    dialog.accepted(markdown.checked ? "markdown" : csv.checked ? "csv" : "json",
                                    subtree.checked ? "subtree" : local.checked ? "local" : "all")
                }
            }
        }
    }
}
