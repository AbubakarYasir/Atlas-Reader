import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

// Composes the application window. Views live in their own files; tokens in
// Theme, strings in I18n (docs/DESIGN.md §11).
ApplicationWindow {
    id: root

    width: 1280
    height: 820
    minimumWidth: 760
    minimumHeight: 560
    visible: true
    color: Theme.canvas
    visibility: AppState.fullScreen ? Window.FullScreen : Window.AutomaticVisibility
    title: readerController.activeIndex >= 0 && reader.currentTab
        ? reader.currentTab.title + " — " + I18n.t("app.name")
        : I18n.t("library.title") + " — " + I18n.t("app.name")

    font.family: I18n.uiFamily
    font.pixelSize: Math.round(14 * I18n.sizeScale)

    palette.window: Theme.canvas
    palette.windowText: Theme.ink
    palette.base: Theme.raised
    palette.alternateBase: Theme.panel
    palette.text: Theme.ink
    palette.button: Theme.raised
    palette.buttonText: Theme.ink
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.onAccent
    palette.placeholderText: Theme.muted
    palette.toolTipBase: Theme.raised
    palette.toolTipText: Theme.ink
    palette.mid: Theme.line
    palette.dark: Theme.controlBorder
    palette.light: Theme.panel

    LayoutMirroring.enabled: I18n.rtl
    LayoutMirroring.childrenInherit: true

    // Bundled fonts (assets/fonts, OFL): Urdu interface and Arabic/Urdu content.
    FontLoader { source: "assets/fonts/NotoNastaliqUrdu-wght.ttf" }
    FontLoader { source: "assets/fonts/NotoNaskhArabic-Regular.ttf" }
    FontLoader { source: "assets/fonts/NotoNaskhArabic-SemiBold.ttf" }

    Binding { target: I18n; property: "language"; value: preferences.language }
    Binding { target: Theme; property: "dark"; value: preferences.dark }
    Binding { target: Theme; property: "touch"; value: preferences.touchMode }
    Binding { target: Theme; property: "scriptExtra"; value: I18n.language === "ur" ? 10 : 0 }

    readonly property bool inReader: readerController.activeIndex >= 0
    property string errorText: ""

    // ---- shortcuts (Foxit-compatible where an equivalent exists; DESIGN.md §7)
    Shortcut { sequences: ["Ctrl+O", "Ctrl+Shift+O"]; onActivated: pdfDialog.open() }
    Shortcut { sequence: "Ctrl+Shift+A"; onActivated: folderDialog.open() }
    Shortcut { sequence: "Ctrl+K"; enabled: !root.inReader; onActivated: libraryView.searchField.forceActiveFocus() }
    Shortcut { sequences: ["Ctrl+W", "Ctrl+F4"]; enabled: root.inReader; onActivated: readerController.closeActive() }
    Shortcut { sequence: "Ctrl+Shift+W"; enabled: readerController.hasSessions; onActivated: readerController.closeAll() }
    Shortcut { sequence: "Ctrl+Tab"; enabled: readerController.hasSessions; onActivated: reader.cycleTab(1) }
    Shortcut { sequence: "Ctrl+Shift+Tab"; enabled: readerController.hasSessions; onActivated: reader.cycleTab(-1) }
    Shortcut { sequence: "Ctrl+G"; enabled: root.inReader && !AppState.chromeHidden; onActivated: if (reader.currentTab) reader.currentTab.focusPageBox() }
    Shortcut { sequence: "Ctrl+H"; enabled: root.inReader; onActivated: AppState.readMode = !AppState.readMode }
    Shortcut { sequence: "F11"; onActivated: AppState.fullScreen = !AppState.fullScreen }
    Shortcut {
        sequence: "Esc"
        enabled: AppState.chromeHidden
        onActivated: { AppState.readMode = false; AppState.fullScreen = false }
    }
    Shortcut { sequence: "F5"; enabled: !root.inReader; onActivated: libraryController.rescan() }

    FolderDialog {
        id: folderDialog
        title: I18n.t("dialog.chooseFolder")
        onAccepted: libraryController.addRoot(selectedFolder)
    }
    FileDialog {
        id: pdfDialog
        title: I18n.t("dialog.openPdf")
        nameFilters: [I18n.t("dialog.pdfFilter") + " (*.pdf)"]
        onAccepted: readerController.openLocalFile(selectedFile)
    }

    Connections {
        target: libraryController
        function onOperationError(message) {
            console.warn("Atlas operation failed:", message)
            root.errorText = I18n.t("error.libraryUnchanged")
            errorPopup.open()
        }
    }

    Popup {
        id: errorPopup
        x: root.LayoutMirroring.enabled ? Theme.xl : root.width - width - Theme.xl
        y: Theme.titleBarHeight + Theme.lg
        width: Math.min(440, root.width - 40)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: Theme.dangerSoft; radius: Theme.radiusControl; border.color: Theme.dangerBorder }
        contentItem: Label {
            text: root.errorText
            color: Theme.danger
            wrapMode: Text.WordWrap
            padding: Theme.md
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            visible: !(AppState.chromeHidden && root.inReader)
            placeTitle: root.inReader && reader.currentTab ? reader.currentTab.title : ""
            onOpenPdfRequested: pdfDialog.open()
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            LibraryView {
                id: libraryView
                anchors.fill: parent
                visible: !root.inReader
                onAddFolderRequested: folderDialog.open()
            }
            ReaderView {
                id: reader
                anchors.fill: parent
                visible: root.inReader
                onOpenPdfRequested: pdfDialog.open()
            }
            AtlasButton {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.lg
                visible: !root.inReader && readerController.hasSessions
                kind: "primary"
                iconName: "book_open"
                text: I18n.t("nav.returnToReader")
                onClicked: readerController.activeIndex = 0
            }
        }
    }
}
