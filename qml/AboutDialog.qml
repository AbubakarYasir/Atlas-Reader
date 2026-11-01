import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Help > About Atlas Reader: version, what Atlas is, and licence notices.
Popup {
    id: dialog
    modal: true
    focus: true
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.min(460, (parent ? parent.width : 500) - 40)
    padding: Theme.xl
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle { color: Theme.raised; radius: Theme.radiusSurface; border.color: Theme.chromeLine }

    contentItem: ColumnLayout {
        Accessible.role: Accessible.Dialog
        Accessible.name: I18n.t("about.title")
        spacing: Theme.md
        RowLayout {
            spacing: Theme.md
            Image {
                source: "assets/branding/atlas-reader-icon.png"
                sourceSize.width: 48
                sourceSize.height: 48
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                Accessible.ignored: true
            }
            ColumnLayout {
                spacing: 0
                Label {
                    text: I18n.t("app.name")
                    color: Theme.ink
                    font.pixelSize: Math.round(20 * I18n.sizeScale)
                    font.weight: Font.DemiBold
                }
                Label {
                    text: I18n.t("about.version", I18n.ltr(atlasVersion))
                    color: Theme.muted
                }
            }
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            text: I18n.t("about.body")
            color: Theme.ink
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            text: I18n.t("about.notices")
            color: Theme.muted
            font.pixelSize: Math.round(12 * I18n.sizeScale)
            wrapMode: Text.WordWrap
        }
        AtlasButton {
            Layout.alignment: Qt.AlignRight
            kind: "primary"
            text: I18n.t("action.close")
            onClicked: dialog.close()
        }
    }
}
