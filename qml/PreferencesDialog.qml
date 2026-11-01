import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Foxit-style Preferences window (Ctrl+K): search box over a category list,
// the selected page on the right, OK / Cancel. Changes preview immediately;
// Cancel restores the values from when the window opened.
Popup {
    id: dialog

    property string page: "general"
    property var snapshot: ({})

    readonly property var categories: [
        { id: "general", key: "prefs.general", words: "startup restore tabs touch mouse" },
        { id: "languages", key: "prefs.languages", words: "language english arabic urdu العربية اردو" },
        { id: "skins", key: "prefs.skins", words: "skin theme dark light colour color" },
        { id: "accessibility", key: "prefs.accessibility", words: "reading colours colors contrast sepia night" }
    ]

    function openPage(name) {
        page = name || "general"
        open()
    }
    function capture() {
        snapshot = {
            language: preferences.language, themeMode: preferences.themeMode, skin: preferences.skin,
            themeColor: preferences.themeColor, touchMode: preferences.touchMode,
            readingColors: preferences.readingColors, restore: readerController.restoreEnabled
        }
    }
    function restore() {
        preferences.language = snapshot.language
        preferences.themeMode = snapshot.themeMode
        preferences.skin = snapshot.skin
        preferences.themeColor = snapshot.themeColor
        preferences.touchMode = snapshot.touchMode
        preferences.readingColors = snapshot.readingColors
        readerController.restoreEnabled = snapshot.restore
    }

    modal: true
    focus: true
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.min(780, (parent ? parent.width : 800) - 40)
    height: Math.min(540, (parent ? parent.height : 600) - 40)
    padding: 0
    closePolicy: Popup.NoAutoClose
    Shortcut { sequence: "Esc"; enabled: dialog.opened; onActivated: { dialog.restore(); dialog.close() } }
    onAboutToShow: { capture(); searchField.text = "" }


    background: Rectangle {
        color: Theme.raised
        radius: Theme.radiusControl
        border.color: Theme.chromeLine
    }

    contentItem: ColumnLayout {
        Accessible.role: Accessible.Dialog
        Accessible.name: I18n.t("prefs.title")
        spacing: 0

        // header
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40 + Theme.scriptExtra
            Layout.leftMargin: Theme.lg
            Layout.rightMargin: Theme.xs
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: I18n.t("prefs.title")
                color: Theme.ink
                font.pixelSize: Math.round(14 * I18n.sizeScale)
            }
            AtlasIconButton {
                iconName: "dismiss"
                iconSize: 16
                text: I18n.t("prefs.cancel")
                onClicked: { dialog.restore(); dialog.close() }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.md
            Layout.rightMargin: Theme.md
            spacing: Theme.sm

            // categories
            ColumnLayout {
                Layout.preferredWidth: 180
                Layout.maximumWidth: 180
                Layout.fillWidth: false
                Layout.fillHeight: true
                spacing: 4
                AtlasTextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: I18n.t("prefs.search")
                    Accessible.name: I18n.t("prefs.search")
                    Keys.onDownPressed: categoryList.forceActiveFocus()
                }
                Shortcut { sequence: "Ctrl+F"; enabled: dialog.opened; onActivated: searchField.forceActiveFocus() }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.raised
                    border.color: Theme.chromeLine
                    ListView {
                        id: categoryList
                        anchors.fill: parent
                        anchors.margins: 1
                        clip: true
                        keyNavigationEnabled: true
                        model: dialog.categories.filter(function(c) {
                            const q = searchField.text.trim().toLowerCase()
                            return q.length === 0 || I18n.t(c.key).toLowerCase().indexOf(q) >= 0 || c.words.indexOf(q) >= 0
                        })
                        delegate: ItemDelegate {
                            id: cat
                            required property var modelData
                            width: ListView.view.width
                            height: 26 + Theme.scriptExtra
                            text: I18n.t(modelData.key)
                            highlighted: dialog.page === modelData.id
                            onClicked: dialog.page = modelData.id
                            Accessible.role: Accessible.ListItem
                            Accessible.selected: highlighted
                            background: Rectangle {
                                color: cat.highlighted ? Theme.accentSoft : cat.hovered ? Theme.hover : "transparent"
                                border.width: cat.visualFocus ? 2 : 0
                                border.color: Theme.focusRing
                            }
                            contentItem: Text {
                                text: cat.text
                                color: Theme.ink
                                font.family: I18n.uiFamily
                                font.pixelSize: Math.round(13 * I18n.sizeScale)
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: categoryList.count === 0
                            text: I18n.t("prefs.noMatch")
                            color: Theme.muted
                        }
                    }
                }
            }

            // page
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.raised
                border.color: Theme.chromeLine
                ScrollView {
                    id: pageScroll
                    anchors.fill: parent
                    anchors.margins: Theme.md
                    contentWidth: availableWidth

                    // General
                    ColumnLayout {
                        visible: dialog.page === "general"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.startup")
                            CheckBox {
                                text: I18n.t("reader.restoreTabs")
                                checked: readerController.restoreEnabled
                                onToggled: readerController.restoreEnabled = checked
                            }
                        }
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.basicTools")
                            CheckBox {
                                text: I18n.t("appearance.touch")
                                checked: preferences.touchMode
                                onToggled: preferences.touchMode = checked
                            }
                        }
                    }

                    // Languages
                    ColumnLayout {
                        visible: dialog.page === "languages"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.languageGroup")
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2
                                Repeater {
                                    model: I18n.languages
                                    delegate: RadioButton {
                                        required property var modelData
                                        text: modelData.name
                                        font.family: modelData.code === "ur" ? "Noto Nastaliq Urdu" : ""
                                        checked: preferences.language === modelData.code
                                        onClicked: preferences.language = modelData.code
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    Layout.topMargin: Theme.sm
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.t("prefs.languageHint")
                                    color: Theme.muted
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    // Skins
                    SkinsPanel {
                        visible: dialog.page === "skins"
                        width: pageScroll.availableWidth
                    }

                    // Accessibility
                    ColumnLayout {
                        visible: dialog.page === "accessibility"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.readingColorsGroup")
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2
                                Repeater {
                                    model: 4
                                    delegate: RadioButton {
                                        required property int index
                                        text: I18n.t("view.colors." + index)
                                        checked: preferences.readingColors === index
                                        onClicked: preferences.readingColors = index
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    Layout.topMargin: Theme.sm
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.t("prefs.readingColorsHint")
                                    color: Theme.muted
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }
        }

        // OK / Cancel
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.md
            spacing: Theme.sm
            Item { Layout.fillWidth: true }
            AtlasButton {
                kind: "primary"
                implicitWidth: 86
                text: I18n.t("prefs.ok")
                onClicked: dialog.close()
            }
            AtlasButton {
                implicitWidth: 86
                text: I18n.t("prefs.cancel")
                onClicked: { dialog.restore(); dialog.close() }
            }
        }
    }
}
