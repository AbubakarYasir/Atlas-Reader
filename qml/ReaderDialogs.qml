import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Reader dialogs (N4.5): Zoom To (Ctrl+M), Word Count and the confirmation
// shown before a document link opens a web page. Shared by every tab.
Item {
    id: dialogs
    anchors.fill: parent

    property var view: null

    function openZoomTo(v) {
        view = v
        zoomField.text = String(Math.round(v.zoom * 100))
        zoomDialog.open()
        zoomField.forceActiveFocus()
        zoomField.selectAll()
    }
    function openWordCount(v) {
        view = v
        wordDialog.words = -1
        wordDialog.open()
        v.requestWordCount()
    }
    function confirmLink(url) {
        linkDialog.url = url
        linkDialog.open()
    }
    function showToast(text) {
        toast.text = text
        toast.open()
        toast.Accessible.announce(text, Accessible.Polite)
        toastTimer.restart()
    }

    Connections {
        target: dialogs.view
        ignoreUnknownSignals: true
        function onWordCountReady(words, characters, pages) {
            wordDialog.words = words
            wordDialog.characters = characters
            wordDialog.pages = pages
        }
    }

    component DialogFrame: Popup {
        modal: true
        focus: true
        x: Math.round((dialogs.width - width) / 2)
        y: Math.round((dialogs.height - height) / 3)
        width: Math.min(420, dialogs.width - 40)
        padding: Theme.xl
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: Theme.raised; radius: Theme.radiusSurface; border.color: Theme.chromeLine }
    }
    component Title: Label {
        Layout.fillWidth: true
        font.family: I18n.uiFamily
        horizontalAlignment: Text.AlignLeft
        color: Theme.ink
        font.pixelSize: Math.round(16 * I18n.sizeScale)
        font.weight: Font.DemiBold
    }

    // ---- Zoom To (Foxit: View → Zoom → Zoom To…)
    DialogFrame {
        id: zoomDialog
        width: Math.min(340, dialogs.width - 40)
        function apply() {
            const value = parseFloat(zoomField.text)
            if (dialogs.view && value > 0) dialogs.view.zoom = Math.max(0.05, Math.min(64, value / 100))
            close()
            if (dialogs.view) dialogs.view.forceActiveFocus()
        }
        contentItem: ColumnLayout {
            Accessible.role: Accessible.Dialog
            Accessible.name: I18n.t("zoom.zoomTo")
            spacing: Theme.md
            Title { text: I18n.t("zoom.zoomTo") }
            RowLayout {
                Label { text: I18n.t("zoom.magnification"); color: Theme.ink }
                AtlasTextField {
                    id: zoomField
                    Layout.preferredWidth: 90
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator { bottom: 5; top: 6400; decimals: 1 }
                    horizontalAlignment: Text.AlignRight
                    Accessible.name: I18n.t("zoom.magnification")
                    onAccepted: zoomDialog.apply()
                }
                Label { text: "%"; color: Theme.muted }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                AtlasButton { text: I18n.t("prefs.cancel"); onClicked: zoomDialog.close() }
                AtlasButton { kind: "primary"; text: I18n.t("prefs.ok"); onClicked: zoomDialog.apply() }
            }
        }
    }

    // ---- Word Count (Foxit: View → Word Count)
    DialogFrame {
        id: wordDialog
        property int words: -1
        property int characters: 0
        property int pages: 0
        contentItem: ColumnLayout {
            Accessible.role: Accessible.Dialog
            Accessible.name: I18n.t("wordCount.title")
            spacing: Theme.md
            Title { text: I18n.t("wordCount.title") }
            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                visible: wordDialog.words < 0
                running: visible
            }
            Label {
                visible: wordDialog.words < 0
                text: I18n.t("wordCount.counting")
                color: Theme.muted
            }
            GridLayout {
                visible: wordDialog.words >= 0
                columns: 2
                columnSpacing: Theme.xl
                rowSpacing: Theme.sm
                Label { text: I18n.t("wordCount.pages"); color: Theme.muted }
                Label { text: I18n.isolate(wordDialog.pages.toLocaleString(Qt.locale(I18n.language), "f", 0)); color: Theme.ink }
                Label { text: I18n.t("wordCount.words"); color: Theme.muted }
                Label { text: I18n.isolate(wordDialog.words.toLocaleString(Qt.locale(I18n.language), "f", 0)); color: Theme.ink }
                Label { text: I18n.t("wordCount.characters"); color: Theme.muted }
                Label { text: I18n.isolate(wordDialog.characters.toLocaleString(Qt.locale(I18n.language), "f", 0)); color: Theme.ink }
            }
            Label {
                Layout.fillWidth: true
                visible: wordDialog.words === 0
                text: I18n.t("find.noText")
                color: Theme.warning
                wrapMode: Text.WordWrap
            }
            AtlasButton {
                Layout.alignment: Qt.AlignRight
                kind: "primary"
                text: I18n.t("action.close")
                onClicked: wordDialog.close()
            }
        }
    }

    // ---- external link confirmation (Foxit: Trust Manager asks before a web link opens)
    DialogFrame {
        id: linkDialog
        property string url: ""
        width: Math.min(480, dialogs.width - 40)
        contentItem: ColumnLayout {
            Accessible.role: Accessible.Dialog
            Accessible.name: I18n.t("link.title")
            spacing: Theme.md
            Title { text: I18n.t("link.title") }
            Label {
                Layout.fillWidth: true
                text: I18n.t("link.body")
                color: Theme.ink
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: urlText.implicitHeight + Theme.md
                color: Theme.sunken
                radius: Theme.radiusControl
                Text {
                    id: urlText
                    anchors.fill: parent
                    anchors.margins: Theme.sm
                    text: I18n.ltr(linkDialog.url)
                    color: Theme.ink
                    font.pixelSize: 12
                    wrapMode: Text.WrapAnywhere
                    maximumLineCount: 4
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    LayoutMirroring.enabled: false
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                AtlasButton {
                    iconName: "copy"
                    text: I18n.t("link.copy")
                    onClicked: { readerController.copyText(linkDialog.url); linkDialog.close() }
                }
                AtlasButton { text: I18n.t("prefs.cancel"); onClicked: linkDialog.close() }
                AtlasButton {
                    kind: "primary"
                    iconName: "open"
                    text: I18n.t("link.open")
                    onClicked: {
                        if (!readerController.openExternalLink(linkDialog.url))
                            dialogs.showToast(I18n.t("link.blocked"))
                        linkDialog.close()
                    }
                }
            }
        }
    }

    // ---- short confirmation toast (snapshot copied, text copied)
    Popup {
        id: toast
        property string text: ""
        x: Math.round((dialogs.width - width) / 2)
        y: dialogs.height - height - 56
        padding: Theme.md
        closePolicy: Popup.NoAutoClose
        background: Rectangle { color: Theme.inkStrong; radius: Theme.radiusControl; opacity: 0.92 }
        contentItem: Label { text: toast.text; color: Theme.raised }
        Timer { id: toastTimer; interval: 1800; onTriggered: toast.close() }
    }
}
