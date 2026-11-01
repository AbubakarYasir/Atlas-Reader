import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import Atlas.Reader

// One open document: page viewport and Foxit-style status bar, plus the
// status view used while loading, for passwords and for errors (DESIGN.md §3).
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

        // ---- status bar (Foxit layout: navigation leading, zoom trailing)
        Item {
            id: statusArea
            Layout.fillWidth: true
            readonly property bool autoHide: preferences.statusBarMode === 1
            readonly property bool shown: !AppState.chromeHidden && preferences.statusBarMode !== 2
                && (!autoHide || statusHover.hovered || pageBox.activeFocus)
            Layout.preferredHeight: AppState.chromeHidden || preferences.statusBarMode === 2 ? 0
                : autoHide ? (shown ? Theme.statusBarHeight : 6) : Theme.statusBarHeight
            visible: Layout.preferredHeight > 0
            HoverHandler { id: statusHover }

        Rectangle {
            anchors.fill: parent
            visible: statusArea.shown
            color: Theme.chrome
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.chromeLine }

            RowLayout {
                anchors.fill: parent
                anchors.topMargin: 1
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 0

                component StatusButton: AtlasIconButton {
                    implicitWidth: 26
                    implicitHeight: 26
                    iconSize: 16
                    cornerRadius: 2
                    mirrorIcon: true
                }

                StatusButton { iconName: "arrow_previous"; text: I18n.t("nav.firstPage"); shortcutHint: "Ctrl+Home"; enabled: view.currentPage > 1; onClicked: view.firstPage() }
                StatusButton { iconName: "chevron_left"; text: I18n.t("nav.previousPage"); shortcutHint: "Ctrl+Page Up"; enabled: view.currentPage > 1; onClicked: view.previousPage() }
                Rectangle { // page box: "n / total" in one field, as in Foxit
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                    implicitWidth: pageRow.implicitWidth + 12
                    implicitHeight: Math.min(24 + Theme.scriptExtra, statusArea.height - 6)
                    radius: 2
                    color: Theme.raised
                    border.width: pageBox.activeFocus ? 2 : 1
                    border.color: pageBox.activeFocus ? Theme.focusRing : Theme.controlBorder
                    Row {
                        id: pageRow
                        anchors.centerIn: parent
                        LayoutMirroring.enabled: false   // "12 / 340" reads left to right in every language
                        spacing: 4
                        TextInput {
                            id: pageBox
                            width: Math.max(18, contentWidth)
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                            color: Theme.ink
                            selectionColor: Theme.accent
                            selectedTextColor: Theme.accentText
                            font.pixelSize: 12
                            selectByMouse: true
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator { bottom: 1; top: Math.max(1, view.pageCount) }
                            Accessible.role: Accessible.EditableText
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
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "/ " + view.pageCount
                            color: Theme.muted
                            font.pixelSize: 12
                        }
                    }
                }
                StatusButton { iconName: "chevron_right"; text: I18n.t("nav.nextPage"); shortcutHint: "Ctrl+Page Down"; enabled: view.currentPage < view.pageCount; onClicked: view.nextPage() }
                StatusButton { iconName: "arrow_next"; text: I18n.t("nav.lastPage"); shortcutHint: "End"; enabled: view.currentPage < view.pageCount; onClicked: view.lastPage() }

                Item { Layout.fillWidth: true }

                StatusButton { iconName: "zoom_out"; mirrorIcon: false; text: I18n.t("zoom.out"); shortcutHint: "Ctrl+-"; onClicked: view.zoomBy(0.8) }
                Slider {
                    id: zoomSlider
                    Layout.preferredWidth: tab.width > 900 ? 120 : 80
                    from: Math.log(0.1)
                    to: Math.log(8)
                    value: Math.log(Math.max(0.1, view.zoom))
                    onMoved: view.zoom = Math.exp(value)
                    Accessible.name: I18n.t("zoom.level")
                    background: Rectangle {
                        x: zoomSlider.leftPadding
                        y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                        width: zoomSlider.availableWidth
                        height: 2
                        color: Theme.controlBorder
                    }
                    handle: Rectangle {
                        x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                        y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                        implicitWidth: 8
                        implicitHeight: 14
                        radius: 2
                        color: zoomSlider.pressed ? Theme.accent : Theme.raised
                        border.width: zoomSlider.visualFocus ? 2 : 1
                        border.color: zoomSlider.visualFocus ? Theme.focusRing : Theme.ink
                    }
                }
                StatusButton { iconName: "zoom_in"; mirrorIcon: false; text: I18n.t("zoom.in"); shortcutHint: "Ctrl+="; onClicked: view.zoomBy(1.25) }
                AbstractButton {
                    id: percentButton
                    implicitWidth: 52
                    implicitHeight: 26
                    hoverEnabled: true
                    Accessible.role: Accessible.ButtonMenu
                    Accessible.name: I18n.t("zoom.percent", Math.round(view.zoom * 100))
                    onClicked: percentMenu.popup(percentButton, 0, -percentMenu.implicitHeight)
                    background: Rectangle { radius: 2; color: percentButton.hovered ? Theme.hover : "transparent" }
                    contentItem: Text {
                        text: I18n.isolate(Math.round(view.zoom * 100) + "%")
                        color: Theme.ink
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Menu {
                        id: percentMenu
                        Repeater {
                            model: [0.5, 0.75, 1, 1.25, 1.5, 2, 4]
                            delegate: MenuItem {
                                required property var modelData
                                text: I18n.isolate(Math.round(modelData * 100) + "%")
                                onTriggered: view.zoom = modelData
                            }
                        }
                        MenuSeparator {}
                        MenuItem { text: I18n.t("zoom.actualSize"); onTriggered: view.actualSize() }
                        MenuItem { text: I18n.t("zoom.fitPage"); onTriggered: view.fitMode = ReaderViewport.Page }
                        MenuItem { text: I18n.t("zoom.fitWidth"); onTriggered: view.fitMode = ReaderViewport.Width }
                    }
                }
                Rectangle { width: 1; Layout.preferredHeight: 16; color: Theme.chromeLine; Layout.leftMargin: 4; Layout.rightMargin: 4 }
                StatusButton { iconName: "full_screen_maximize"; mirrorIcon: false; text: I18n.t("cmd.fullScreen"); shortcutHint: "F11"; onClicked: AppState.fullScreen = true }
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
