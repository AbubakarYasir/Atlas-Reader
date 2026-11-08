import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Save into the PDF (N5.4, ADR-0009): shown before the first save of a book in
// this session. Says what will happen in plain words and offers the one real
// choice (also remove the PDF bookmarks deleted in Atlas). Later saves of the
// same book go straight ahead with the same choice.
Popup {
    id: dialog

    required property var bookmarks     // BookmarkModel
    signal accepted(bool removeHidden)

    property var preview: ({})
    function ask() {
        removeHidden.checked = false
        preview = bookmarks.savePreview(false)
        open()
    }
    readonly property var current: removeHidden.checked ? bookmarks.savePreview(true) : preview

    modal: true
    focus: true
    parent: Overlay.overlay
    x: Math.round(((parent ? parent.width : 800) - width) / 2)
    y: Math.round(((parent ? parent.height : 600) - height) / 3)
    width: Math.min(460, (parent ? parent.width : 800) - 40)
    height: Math.min(body.implicitHeight + footer.implicitHeight + Theme.md + padding * 2,
                     (parent ? parent.height : 600) - Theme.xl * 2)
    padding: Theme.xl
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: Theme.raised; radius: Theme.radiusSurface; border.color: Theme.chromeLine }

    contentItem: ColumnLayout {
        spacing: Theme.md
        Accessible.role: Accessible.Dialog
        Accessible.name: I18n.t("save.title")

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
                    text: I18n.t("save.title")
                    color: Theme.ink
                    font.family: I18n.uiFamily
                    font.pixelSize: Math.round(16 * I18n.sizeScale)
                    font.weight: Font.DemiBold
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    text: I18n.t("save.summary", dialog.current.added || 0, dialog.current.changed || 0, dialog.current.kept || 0)
                    color: Theme.ink
                    font.family: I18n.uiFamily
                }
                AtlasCheckBox {
                    id: removeHidden
                    Layout.fillWidth: true
                    visible: dialog.bookmarks.hiddenCount > 0
                    text: I18n.t("save.removeHidden", dialog.bookmarks.hiddenCount)
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    text: I18n.t("save.safety")
                    color: Theme.muted
                    font.family: I18n.uiFamily
                    font.pixelSize: Math.round(12 * I18n.sizeScale)
                }
            }
        }
        RowLayout {
            id: footer
            Layout.alignment: Qt.AlignRight
            spacing: Theme.sm
            AtlasButton { text: I18n.t("prefs.cancel"); onClicked: dialog.close() }
            AtlasButton {
                kind: "primary"
                iconName: "save"
                text: I18n.t("save.action")
                enabled: !dialog.current.refusal
                onClicked: { dialog.close(); dialog.accepted(removeHidden.checked) }
            }
        }
    }
}
