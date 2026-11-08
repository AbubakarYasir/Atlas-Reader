import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts

// Foxit-style title bar (FOXIT_PARITY §1): 48 px, app mark, Quick Access
// Toolbar, file name, then window buttons. Colourful skin tints it with the
// theme colour. Dragging moves the window through the system (keeps Windows
// snap); double-click maximizes. With --native-frame the system draws the
// caption and this bar hides its window buttons.
Rectangle {
    id: bar

    signal openPdfRequested()
    signal saveRequested()
    signal undoRequested()
    signal redoRequested()
    property string placeTitle: ""
    property bool canSave: false
    property bool canUndo: false
    property bool canRedo: false
    property bool hasDocument: false
    property var bookmarks: null
    signal commandRequested(string command)
    signal bookRequested(string documentId, string path)
    signal bookmarkRequested(var found)
    // Command Center (Alt+Q), parked right of the file name like Foxit's search.
    readonly property alias commandCenter: center
    function openSearch() { center.open() }
    property bool customFrame: true
    readonly property var win: Window.window
    readonly property bool maximized: win !== null && win.visibility === Window.Maximized

    implicitHeight: Theme.titleBarHeight
    color: Theme.titleBar
    Accessible.role: Accessible.TitleBar
    Accessible.name: I18n.t("app.name")

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.titleBarTinted ? Qt.darker(Theme.titleBar, 1.15) : Theme.chromeLine }

    // Drag area underneath the controls.
    MouseArea {
        anchors.fill: parent
        enabled: bar.customFrame
        acceptedButtons: Qt.LeftButton
        onPressed: if (bar.win) bar.win.startSystemMove()
        onDoubleClicked: if (bar.win) bar.win.visibility = bar.maximized ? Window.Windowed : Window.Maximized
    }

    RowLayout {
        anchors.fill: parent
        anchors.bottomMargin: 1
        spacing: 0

        Image {
            Layout.leftMargin: 12
            Layout.rightMargin: 11
            source: "assets/branding/atlas-reader-icon.png"
            sourceSize.width: 18
            sourceSize.height: 18
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            fillMode: Image.PreserveAspectFit
            smooth: true
            Accessible.ignored: true
        }
        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 16; color: Qt.alpha(Theme.titleBarText, 0.45) }

        // Quick Access Toolbar
        Row {
            Layout.leftMargin: 6
            spacing: 0
            AtlasIconButton {
                implicitWidth: Theme.qatButton
                implicitHeight: Theme.qatButton
                iconSize: 16
                iconName: "folder_open"
                iconColor: Theme.titleBarText
                hoverColor: Theme.titleBarHover
                cornerRadius: 2
                text: I18n.t("backstage.open")
                shortcutHint: "Ctrl+O"
                onClicked: bar.openPdfRequested()
            }
            // Save puts this book's bookmarks into the PDF (N5.4); Undo/Redo
            // act on its bookmark edits (N5.3).
            AtlasIconButton {
                implicitWidth: Theme.qatButton
                implicitHeight: Theme.qatButton
                iconSize: 16
                iconName: "save"
                iconColor: Theme.titleBarText
                hoverColor: Theme.titleBarHover
                cornerRadius: 2
                enabled: bar.canSave
                text: I18n.t("save.qat")
                shortcutHint: "Ctrl+S"
                onClicked: bar.saveRequested()
            }
            AtlasIconButton {
                implicitWidth: Theme.qatButton
                implicitHeight: Theme.qatButton
                iconSize: 16
                iconName: "arrow_undo"
                iconColor: Theme.titleBarText
                hoverColor: Theme.titleBarHover
                cornerRadius: 2
                enabled: bar.canUndo
                text: I18n.t("bookmarks.undo")
                shortcutHint: "Ctrl+Z"
                onClicked: bar.undoRequested()
            }
            AtlasIconButton {
                implicitWidth: Theme.qatButton
                implicitHeight: Theme.qatButton
                iconSize: 16
                iconName: "arrow_redo"
                iconColor: Theme.titleBarText
                hoverColor: Theme.titleBarHover
                cornerRadius: 2
                enabled: bar.canRedo
                text: I18n.t("bookmarks.redo")
                shortcutHint: "Ctrl+Y"
                onClicked: bar.redoRequested()
            }
            AtlasIconButton {
                implicitWidth: Theme.qatButton
                implicitHeight: Theme.qatButton
                iconSize: 16
                iconName: preferences.touchMode ? "tablet" : "hand_left"
                iconColor: Theme.titleBarText
                hoverColor: Theme.titleBarHover
                cornerRadius: 2
                text: I18n.t("cmd.mouseTouch")
                onClicked: preferences.touchMode = !preferences.touchMode
            }
        }
        Rectangle { Layout.leftMargin: 6; Layout.preferredWidth: 1; Layout.preferredHeight: 16; color: Qt.alpha(Theme.titleBarText, 0.45) }

        IconImage {
            Layout.leftMargin: 14
            source: Theme.icon(bar.placeTitle.length > 0 ? "document" : "library")
            sourceSize.width: 16
            sourceSize.height: 16
            color: Theme.titleBarText
        }
        Label {
            Layout.leftMargin: 8
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            text: bar.placeTitle.length > 0 ? bar.placeTitle : I18n.t("library.title")
            color: Theme.titleBarText
            elide: Text.ElideMiddle
            font.family: I18n.contentFamily(text) || I18n.uiFamily
            font.pixelSize: Math.round(12 * I18n.sizeScale)
        }

        CommandCenter {
            id: center
            Layout.preferredWidth: implicitWidth
            Layout.preferredHeight: Theme.qatButton
            Layout.leftMargin: Theme.sm
            Layout.rightMargin: bar.customFrame ? 6 : 10
            canUndo: bar.canUndo
            canRedo: bar.canRedo
            hasDocument: bar.hasDocument
            bookmarks: bar.bookmarks
            onCommandRequested: (command) => bar.commandRequested(command)
            onBookRequested: (documentId, path) => bar.bookRequested(documentId, path)
            onBookmarkRequested: (found) => bar.bookmarkRequested(found)
        }
        Rectangle { visible: bar.customFrame; Layout.rightMargin: 4; Layout.preferredWidth: 1; Layout.preferredHeight: 16; color: Qt.alpha(Theme.titleBarText, 0.45) }

        // Window buttons (minimize, maximize/restore, close).
        Row {
            visible: bar.customFrame
            Layout.fillHeight: true
            spacing: 0
            Repeater {
                model: [
                    { icon: "subtract", key: "window.minimize", action: 0 },
                    { icon: bar.maximized ? "square_multiple" : "maximize", key: bar.maximized ? "window.restore" : "window.maximize", action: 1 },
                    { icon: "dismiss", key: "window.close", action: 2 }
                ]
                delegate: AbstractButton {
                    id: caption
                    required property var modelData
                    width: Theme.captionButton
                    height: bar.height - 1
                    hoverEnabled: true
                    focusPolicy: Qt.NoFocus
                    Accessible.role: Accessible.Button
                    Accessible.name: I18n.t(modelData.key)
                    ToolTip.visible: hovered
                    ToolTip.delay: 700
                    ToolTip.text: I18n.t(modelData.key)
                    onClicked: {
                        if (!bar.win) return
                        if (modelData.action === 0) bar.win.showMinimized()
                        else if (modelData.action === 1) bar.win.visibility = bar.maximized ? Window.Windowed : Window.Maximized
                        else bar.win.close()
                    }
                    background: Rectangle {
                        color: caption.modelData.action === 2 && caption.hovered ? Theme.closeHover
                            : caption.down ? Qt.darker(Theme.titleBarHover, 1.1)
                            : caption.hovered ? Theme.titleBarHover : "transparent"
                    }
                    contentItem: Item {
                        IconImage {
                            anchors.centerIn: parent
                            source: Theme.icon(caption.modelData.icon)
                            sourceSize.width: 16
                            sourceSize.height: 16
                            color: caption.modelData.action === 2 && caption.hovered ? Theme.closeHoverText : Theme.titleBarText
                        }
                    }
                }
            }
        }
    }
}
