import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Find bar (Foxit Find, Ctrl+F; N4.5): floats at the top trailing corner of
// the document. Enter / F3 next, Shift+Enter / Shift+F3 previous, Esc closes.
// Options (Whole words only, Case-Sensitive) are shared with the Search panel.
Rectangle {
    id: bar

    required property var view          // ReaderViewport
    signal closed()
    signal advancedRequested()

    function open(text) {
        visible = true
        if (text && text.length > 0) field.text = text
        field.forceActiveFocus()
        field.selectAll()
    }
    function run() {
        if (field.text.trim().length === 0) { view.clearSearch(); return }
        if (field.text.trim() !== view.searchQuery || !view.searchHits.length && !view.searching)
            view.find(field.text, preferences.searchMatchCase, preferences.searchWholeWords)
        else
            view.findNext()
    }
    function close() {
        visible = false
        view.clearSearch()
        view.forceActiveFocus()
        closed()
    }

    visible: false
    width: row.implicitWidth + 12
    height: 36
    radius: Theme.radiusControl
    color: Theme.raised
    border.color: Theme.line
    Accessible.role: Accessible.ToolBar
    Accessible.name: I18n.t("find.title")

    // Re-run when the options change.
    Connections {
        target: preferences
        function onSearchOptionsChanged() {
            if (bar.visible && field.text.trim().length > 0)
                bar.view.find(field.text, preferences.searchMatchCase, preferences.searchWholeWords)
        }
    }

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 2

        AtlasTextField {
            id: field
            Layout.preferredWidth: 220
            implicitHeight: 28
            placeholderText: I18n.t("find.placeholder")
            selectByMouse: true
            font.family: I18n.contentFamily(text) || I18n.uiFamily
            Accessible.name: I18n.t("find.title")
            Accessible.description: "Ctrl+F"
            onAccepted: bar.run()
            Keys.onPressed: (event) => {
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ShiftModifier)) {
                    bar.view.findPrevious(); event.accepted = true
                } else if (event.key === Qt.Key_Escape) {
                    bar.close(); event.accepted = true
                }
            }
        }
        Label {
            Layout.minimumWidth: 64
            horizontalAlignment: Text.AlignHCenter
            color: bar.view.searchQuery.length > 0 && !bar.view.searching && bar.view.searchHits.length === 0 ? Theme.danger : Theme.muted
            font.pixelSize: 12
            text: bar.view.searchQuery.length === 0 ? ""
                : bar.view.searching && bar.view.searchHits.length === 0 ? I18n.t("find.searching")
                : bar.view.searchHits.length === 0 ? I18n.t("find.noResults")
                : I18n.t("find.count", bar.view.searchIndex + 1, bar.view.searchHits.length)
        }
        AtlasIconButton {
            implicitWidth: 28; implicitHeight: 28; iconSize: 16; cornerRadius: 2
            iconName: "arrow_up"
            text: I18n.t("find.previous")
            shortcutHint: "Shift+F3"
            enabled: bar.view.searchHits.length > 0
            onClicked: bar.view.findPrevious()
        }
        AtlasIconButton {
            implicitWidth: 28; implicitHeight: 28; iconSize: 16; cornerRadius: 2
            iconName: "arrow_down"
            text: I18n.t("find.next")
            shortcutHint: "F3"
            enabled: field.text.trim().length > 0
            onClicked: bar.run()
        }
        AtlasIconButton {
            id: optionsButton
            implicitWidth: 28; implicitHeight: 28; iconSize: 16; cornerRadius: 2
            iconName: "options"
            text: I18n.t("find.options")
            onClicked: optionsMenu.popup(optionsButton, 0, optionsButton.height)
            Menu {
                id: optionsMenu
                MenuItem {
                    text: I18n.t("find.wholeWords")
                    checkable: true
                    checked: preferences.searchWholeWords
                    onTriggered: preferences.searchWholeWords = !preferences.searchWholeWords
                }
                MenuItem {
                    text: I18n.t("find.matchCase")
                    checkable: true
                    checked: preferences.searchMatchCase
                    onTriggered: preferences.searchMatchCase = !preferences.searchMatchCase
                }
                MenuSeparator {}
                MenuItem { text: I18n.t("find.advanced") + "\tCtrl+Shift+F"; onTriggered: bar.advancedRequested() }
            }
        }
        AtlasIconButton {
            implicitWidth: 28; implicitHeight: 28; iconSize: 14; cornerRadius: 2
            iconName: "dismiss"
            text: I18n.t("action.close")
            shortcutHint: "Esc"
            onClicked: bar.close()
        }
    }
}
