import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Atlas.Reader

// One open document: command bar, page viewport, status bar, and the status
// view used while loading, for passwords and for errors (DESIGN.md §3).
Rectangle {
    id: tab

    required property int index
    required property string title
    required property string sourcePath
    required property string status
    required property int pageCount
    required property string detail
    required property string documentTitle
    required property var sessionId

    readonly property bool ready: status === "ready"
    readonly property alias viewport: view

    color: ready ? Theme.readerBackdrop : Theme.canvas

    function focusPageBox() { pageBox.forceActiveFocus() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: tab.ready

        // ---- command bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.commandBarHeight
            visible: !AppState.chromeHidden
            color: Theme.panel
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sm
                anchors.rightMargin: Theme.sm
                spacing: 2

                AtlasIconButton {
                    iconName: "arrow_rotate_counterclockwise"
                    text: I18n.t("view.rotateLeft")
                    shortcutHint: "Ctrl+Shift+-"
                    onClicked: view.rotateView(-1)
                }
                AtlasIconButton {
                    iconName: "arrow_rotate_clockwise"
                    text: I18n.t("view.rotateRight")
                    shortcutHint: "Ctrl+Shift++"
                    onClicked: view.rotateView(1)
                }
                Rectangle { width: 1; Layout.preferredHeight: Theme.commandBarHeight - 16; color: Theme.line; Layout.leftMargin: Theme.xs; Layout.rightMargin: Theme.xs }
                AtlasButton {
                    kind: "quiet"
                    iconName: "book_open"
                    text: tab.width > 900 ? I18n.t("view.readMode") : ""
                    Accessible.name: I18n.t("view.readMode")
                    Accessible.description: "Ctrl+H"
                    ToolTip.visible: hovered && text.length === 0
                    ToolTip.text: I18n.t("view.readMode") + "  (Ctrl+H)"
                    onClicked: AppState.readMode = true
                }
                AtlasButton {
                    kind: "quiet"
                    iconName: "full_screen_maximize"
                    text: tab.width > 900 ? I18n.t("view.fullScreen") : ""
                    Accessible.name: I18n.t("view.fullScreen")
                    Accessible.description: "F11"
                    ToolTip.visible: hovered && text.length === 0
                    ToolTip.text: I18n.t("view.fullScreen") + "  (F11)"
                    onClicked: AppState.fullScreen = true
                }
                AtlasButton {
                    kind: "quiet"
                    iconName: "color"
                    text: tab.width > 900 ? I18n.t("view.colors." + preferences.readingColors) : ""
                    Accessible.name: I18n.t("view.readingColors")
                    ToolTip.visible: hovered
                    ToolTip.text: I18n.t("view.readingColors")
                    onClicked: colorsMenu.open()
                    Menu {
                        id: colorsMenu
                        y: parent.height
                        Repeater {
                            model: 4
                            delegate: MenuItem {
                                required property int index
                                text: I18n.t("view.colors." + index)
                                checkable: true
                                checked: preferences.readingColors === index
                                onTriggered: preferences.readingColors = index
                            }
                        }
                    }
                }
                Item { Layout.fillWidth: true }
                Label {
                    Layout.maximumWidth: tab.width * 0.35
                    visible: tab.documentTitle.length > 0 && tab.documentTitle !== tab.title && tab.width > 1000
                    text: tab.documentTitle
                    color: Theme.muted
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
                AtlasIconButton {
                    iconName: "more_horizontal"
                    text: I18n.t("action.more")
                    onClicked: moreMenu.open()
                    Menu {
                        id: moreMenu
                        y: parent.height
                        MenuItem {
                            text: I18n.t("reader.restoreTabs")
                            checkable: true
                            checked: readerController.restoreEnabled
                            onToggled: readerController.restoreEnabled = checked
                        }
                        MenuSeparator {}
                        MenuItem { text: I18n.t("tab.closeThis"); onTriggered: readerController.closeAt(tab.index) }
                        MenuItem { text: I18n.t("tab.closeAll"); onTriggered: readerController.closeAll() }
                    }
                }
            }
        }

        // ---- document pane
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ReaderViewport {
                id: view
                anchors.fill: parent
                controller: readerController
                sessionId: tab.sessionId
                active: tab.visible && tab.ready
                readingColors: preferences.readingColors
                pageColor: Theme.readingPaper[preferences.readingColors]
                pageShadowColor: Theme.pageShadow
                focus: tab.visible
                Accessible.role: Accessible.Pane
                Accessible.name: I18n.t("reader.pagesA11y", view.currentPage, view.pageCount)
            }

            ScrollBar {
                id: vbar
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                orientation: Qt.Vertical
                policy: view.contentHeight > view.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                size: view.contentHeight > 0 ? view.height / view.contentHeight : 1
                position: view.contentHeight > 0 ? view.contentY / view.contentHeight : 0
                onPositionChanged: if (pressed) view.contentY = position * view.contentHeight
            }
            ScrollBar {
                anchors.left: parent.left
                anchors.right: vbar.left
                anchors.bottom: parent.bottom
                orientation: Qt.Horizontal
                policy: view.contentWidth > view.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                size: view.contentWidth > 0 ? view.width / view.contentWidth : 1
                position: view.contentWidth > 0 ? view.contentX / view.contentWidth : 0
                onPositionChanged: if (pressed) view.contentX = position * view.contentWidth
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: view.loading
                visible: running
            }
            Label {
                anchors.centerIn: parent
                width: Math.min(520, parent.width - 48)
                visible: view.errorKey.length > 0
                text: I18n.t("reader.renderError")
                color: Theme.ink
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            // ---- read mode / full screen mini bar
            Rectangle {
                id: miniBar
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Theme.lg
                visible: AppState.chromeHidden
                opacity: miniHover.hovered || miniTimer.running || miniRow.activeFocus ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.normal } }
                width: miniRow.implicitWidth + Theme.lg
                height: Theme.commandBarHeight
                radius: Theme.radiusSurface
                color: Theme.panel
                border.color: Theme.line
                HoverHandler { id: miniHover }
                Timer { id: miniTimer; interval: 2200 }
                Connections {
                    target: view
                    function onCurrentPageChanged() { if (AppState.chromeHidden) miniTimer.restart() }
                }
                Connections {
                    target: AppState
                    function onChromeHiddenChanged() { if (AppState.chromeHidden) miniTimer.restart() }
                }
                RowLayout {
                    id: miniRow
                    anchors.centerIn: parent
                    spacing: 2
                    AtlasIconButton { iconName: "chevron_left"; mirrorIcon: true; text: I18n.t("nav.previousPage"); onClicked: view.previousPage() }
                    Label {
                        text: I18n.t("status.pageOf", view.currentPage, view.pageCount)
                        color: Theme.ink
                        font.pixelSize: 13
                        Layout.leftMargin: Theme.xs
                        Layout.rightMargin: Theme.xs
                    }
                    AtlasIconButton { iconName: "chevron_right"; mirrorIcon: true; text: I18n.t("nav.nextPage"); onClicked: view.nextPage() }
                    AtlasIconButton { iconName: "zoom_out"; text: I18n.t("zoom.out"); onClicked: view.zoomBy(0.8) }
                    AtlasIconButton { iconName: "zoom_in"; text: I18n.t("zoom.in"); onClicked: view.zoomBy(1.25) }
                    AtlasButton {
                        kind: "quiet"
                        iconName: "full_screen_minimize"
                        text: AppState.fullScreen ? I18n.t("view.exitFullScreen") : I18n.t("view.exitReadMode")
                        Accessible.description: "Esc"
                        onClicked: { AppState.readMode = false; AppState.fullScreen = false }
                    }
                }
            }
        }

        // ---- status bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.statusBarHeight
            visible: !AppState.chromeHidden
            color: Theme.panel
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.line }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sm
                anchors.rightMargin: Theme.sm
                spacing: 2

                AtlasIconButton { iconName: "arrow_previous"; mirrorIcon: true; text: I18n.t("nav.firstPage"); shortcutHint: "Ctrl+Home"; enabled: view.currentPage > 1; onClicked: view.firstPage() }
                AtlasIconButton { iconName: "chevron_left"; mirrorIcon: true; text: I18n.t("nav.previousPage"); shortcutHint: "Ctrl+Page Up"; enabled: view.currentPage > 1; onClicked: view.previousPage() }
                AtlasTextField {
                    id: pageBox
                    Layout.preferredWidth: 56
                    implicitHeight: Theme.controlHeight - 4
                    horizontalAlignment: Text.AlignHCenter
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: Math.max(1, view.pageCount) }
                    Accessible.name: I18n.t("nav.goToPage")
                    Accessible.description: "Ctrl+G"
                    Binding on text {
                        when: !pageBox.activeFocus
                        value: String(view.currentPage)
                    }
                    onAccepted: { view.goToPage(parseInt(text)); view.forceActiveFocus() }
                    onActiveFocusChanged: if (activeFocus) selectAll()
                    Keys.onEscapePressed: view.forceActiveFocus()
                }
                Label {
                    text: I18n.t("status.ofPages", view.pageCount)
                    color: Theme.muted
                    font.pixelSize: 13
                    Layout.leftMargin: Theme.xs
                    Layout.rightMargin: Theme.xs
                }
                AtlasIconButton { iconName: "chevron_right"; mirrorIcon: true; text: I18n.t("nav.nextPage"); shortcutHint: "Ctrl+Page Down"; enabled: view.currentPage < view.pageCount; onClicked: view.nextPage() }
                AtlasIconButton { iconName: "arrow_next"; mirrorIcon: true; text: I18n.t("nav.lastPage"); shortcutHint: "Ctrl+End"; enabled: view.currentPage < view.pageCount; onClicked: view.lastPage() }

                Item { Layout.fillWidth: true }

                AtlasIconButton {
                    iconName: "ratio_one_to_one"
                    text: I18n.t("zoom.actualSize")
                    shortcutHint: "Ctrl+1"
                    checkable: true
                    checked: view.fitMode === ReaderViewport.None && Math.abs(view.zoom - 1) < 0.001
                    onClicked: view.actualSize()
                }
                AtlasIconButton {
                    iconName: "arrow_autofit_width"
                    text: I18n.t("zoom.fitWidth")
                    shortcutHint: "Ctrl+2"
                    checkable: true
                    checked: view.fitMode === ReaderViewport.Width
                    onClicked: view.fitMode = ReaderViewport.Width
                }
                AtlasIconButton {
                    iconName: "page_fit"
                    text: I18n.t("zoom.fitPage")
                    shortcutHint: "Ctrl+0"
                    checkable: true
                    checked: view.fitMode === ReaderViewport.Page
                    onClicked: view.fitMode = ReaderViewport.Page
                }
                Rectangle { width: 1; Layout.preferredHeight: Theme.statusBarHeight - 16; color: Theme.line; Layout.leftMargin: Theme.xs; Layout.rightMargin: Theme.xs }
                AtlasIconButton { iconName: "zoom_out"; text: I18n.t("zoom.out"); shortcutHint: "Ctrl+-"; onClicked: view.zoomBy(0.8) }
                Slider {
                    id: zoomSlider
                    Layout.preferredWidth: tab.width > 900 ? 140 : 90
                    from: Math.log(0.1)
                    to: Math.log(8)
                    value: Math.log(Math.max(0.1, view.zoom))
                    onMoved: view.zoom = Math.exp(value)
                    Accessible.name: I18n.t("zoom.level")
                    background: Rectangle {
                        x: zoomSlider.leftPadding
                        y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                        width: zoomSlider.availableWidth
                        height: 4
                        radius: 2
                        color: Theme.line
                        Rectangle {
                            x: zoomSlider.mirrored ? parent.width - width : 0
                            width: zoomSlider.position * parent.width
                            height: parent.height
                            radius: 2
                            color: Theme.accent
                        }
                    }
                    handle: Rectangle {
                        x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                        y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 8
                        color: zoomSlider.pressed ? Theme.pressed : Theme.raised
                        border.width: zoomSlider.visualFocus ? 3 : 2
                        border.color: zoomSlider.visualFocus ? Theme.focusRing : Theme.accent
                    }
                }
                AtlasIconButton { iconName: "zoom_in"; text: I18n.t("zoom.in"); shortcutHint: "Ctrl+="; onClicked: view.zoomBy(1.25) }
                Label {
                    Layout.preferredWidth: 48
                    horizontalAlignment: Text.AlignHCenter
                    text: I18n.isolate(Math.round(view.zoom * 100) + "%")
                    color: Theme.ink
                    font.pixelSize: 13
                    Accessible.name: I18n.t("zoom.percent", Math.round(view.zoom * 100))
                }
            }
        }
    }

    // ---- status view: loading, password and errors
    ColumnLayout {
        visible: !tab.ready
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 56)
        spacing: Theme.md

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status === "loading"
            running: visible
        }
        IconImage {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status !== "loading"
            source: Theme.icon(tab.status === "passwordRequired" ? "lock_closed" : "warning")
            sourceSize.width: 40
            sourceSize.height: 40
            color: tab.status === "passwordRequired" ? Theme.accent : Theme.warning
        }
        Label {
            Layout.fillWidth: true
            text: tab.title
            color: Theme.ink
            font.pixelSize: 22
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
        }
        Label {
            Layout.fillWidth: true
            text: tab.status === "loading" ? I18n.t("reader.state.loading")
                : tab.status === "passwordRequired"
                    ? (tab.detail === "incorrect-password" ? I18n.t("reader.state.wrongPassword") : I18n.t("reader.state.password"))
                : tab.status === "missing" ? I18n.t("reader.state.missing")
                : tab.status === "unsupportedSecurity" ? I18n.t("reader.state.unsupportedSecurity")
                : I18n.t("reader.state.malformed")
            color: tab.detail === "incorrect-password" ? Theme.danger : Theme.muted
            font.family: I18n.uiFamily
            font.pixelSize: Math.round(15 * I18n.sizeScale)
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
        AtlasTextField {
            id: passwordField
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(360, parent.width)
            visible: tab.status === "passwordRequired"
            placeholderText: I18n.t("reader.passwordPlaceholder")
            echoMode: TextInput.Password
            selectByMouse: true
            Accessible.name: I18n.t("reader.passwordPlaceholder")
            Accessible.description: I18n.t("reader.passwordNotSaved")
            onVisibleChanged: {
                if (visible) forceActiveFocus()
                else text = ""
            }
            onAccepted: {
                if (text.length === 0) return
                const submitted = text
                text = ""
                readerController.submitPassword(tab.index, submitted)
            }
        }
        AtlasButton {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status === "passwordRequired"
            enabled: passwordField.text.length > 0
            kind: "primary"
            iconName: "lock_closed"
            text: I18n.t("reader.unlock")
            Accessible.description: I18n.t("reader.passwordNotSaved")
            onClicked: {
                const submitted = passwordField.text
                passwordField.text = ""
                readerController.submitPassword(tab.index, submitted)
            }
        }
        Label {
            Layout.fillWidth: true
            visible: tab.status === "passwordRequired"
            text: I18n.t("reader.passwordNotSaved")
            color: Theme.muted
            font.pixelSize: Math.round(12 * I18n.sizeScale)
            horizontalAlignment: Text.AlignHCenter
        }
        Label {
            Layout.fillWidth: true
            text: I18n.ltr(tab.sourcePath)
            color: Theme.muted
            font.pixelSize: 12
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: tab.status !== "loading"
            AtlasButton {
                visible: tab.status !== "passwordRequired"
                kind: "primary"
                text: I18n.t("action.tryAgain")
                onClicked: readerController.retryAt(tab.index)
            }
            AtlasButton {
                text: I18n.t("tab.closeThis")
                onClicked: readerController.closeAt(tab.index)
            }
        }
    }
}
