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
        { id: "documents", key: "prefs.documents", words: "open restore position page last view" },
        { id: "history", key: "prefs.history", words: "recent history clear" },
        { id: "languages", key: "prefs.languages", words: "language english arabic urdu العربية اردو" },
        { id: "pageDisplay", key: "prefs.pageDisplay", words: "page layout zoom facing continuous single fit" },
        { id: "search", key: "prefs.search.title", words: "search find case whole words arabic" },
        { id: "skins", key: "prefs.skins", words: "skin theme dark light colour color" },
        { id: "accessibility", key: "prefs.accessibility", words: "reading colours colors contrast sepia night" },
        { id: "identity", key: "prefs.identity", words: "identity name organization organisation email author" },
        { id: "security", key: "prefs.security", words: "security protected signed signature password permissions check" }
    ]
    // Foxit lists categories alphabetically, in the interface language.
    readonly property var sortedCategories: {
        const language = I18n.language   // re-sort when the language changes
        return categories.slice().sort((a, b) => I18n.t(a.key).localeCompare(I18n.t(b.key), language))
    }

    function openPage(name) {
        page = name || "general"
        open()
    }
    function capture() {
        snapshot = {
            language: preferences.language, themeMode: preferences.themeMode, skin: preferences.skin,
            themeColor: preferences.themeColor, touchMode: preferences.touchMode,
            readingColors: preferences.readingColors, restore: readerController.restoreEnabled,
            defaultPageDisplay: preferences.defaultPageDisplay, defaultZoom: preferences.defaultZoom,
            rememberPosition: preferences.rememberPosition, matchCase: preferences.searchMatchCase,
            wholeWords: preferences.searchWholeWords,
            recheckOnReturn: preferences.recheckOnReturn, showDocumentStatus: preferences.showDocumentStatus
        }
        identityName.text = preferences.identityName
        identityOrganization.text = preferences.identityOrganization
        identityEmail.text = preferences.identityEmail
    }
    function restore() {
        preferences.language = snapshot.language
        preferences.themeMode = snapshot.themeMode
        preferences.skin = snapshot.skin
        preferences.themeColor = snapshot.themeColor
        preferences.touchMode = snapshot.touchMode
        preferences.readingColors = snapshot.readingColors
        readerController.restoreEnabled = snapshot.restore
        preferences.defaultPageDisplay = snapshot.defaultPageDisplay
        preferences.defaultZoom = snapshot.defaultZoom
        preferences.rememberPosition = snapshot.rememberPosition
        preferences.searchMatchCase = snapshot.matchCase
        preferences.searchWholeWords = snapshot.wholeWords
        preferences.recheckOnReturn = snapshot.recheckOnReturn
        preferences.showDocumentStatus = snapshot.showDocumentStatus
    }
    // Identity is typed, not previewed: it is kept only on OK.
    function accept() {
        preferences.identityName = identityName.text.trim()
        preferences.identityOrganization = identityOrganization.text.trim()
        preferences.identityEmail = identityEmail.text.trim()
        close()
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
                        model: dialog.sortedCategories.filter(function(c) {
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
                            AtlasCheckBox {
                                text: I18n.t("reader.restoreTabs")
                                checked: readerController.restoreEnabled
                                onToggled: readerController.restoreEnabled = checked
                            }
                        }
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.basicTools")
                            AtlasCheckBox {
                                text: I18n.t("appearance.touch")
                                checked: preferences.touchMode
                                onToggled: preferences.touchMode = checked
                            }
                        }
                    }

                    // Documents
                    ColumnLayout {
                        visible: dialog.page === "documents"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.openSetting")
                            ColumnLayout {
                                anchors.fill: parent
                                AtlasCheckBox {
                                    text: I18n.t("prefs.rememberPosition")
                                    checked: preferences.rememberPosition
                                    onToggled: preferences.rememberPosition = checked
                                }
                                AtlasCheckBox {
                                    text: I18n.t("reader.restoreTabs")
                                    checked: readerController.restoreEnabled
                                    onToggled: readerController.restoreEnabled = checked
                                }
                            }
                        }
                    }

                    // History
                    ColumnLayout {
                        visible: dialog.page === "history"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.history")
                            ColumnLayout {
                                anchors.fill: parent
                                Label {
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.t("prefs.historyHint", preferences.recentFiles.length)
                                    color: Theme.ink
                                    wrapMode: Text.WordWrap
                                }
                                AtlasButton {
                                    iconName: "history"
                                    text: I18n.t("prefs.clearHistory")
                                    enabled: preferences.recentFiles.length > 0
                                    onClicked: preferences.clearHistory()
                                }
                            }
                        }
                    }

                    // Page Display
                    ColumnLayout {
                        visible: dialog.page === "pageDisplay"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.defaultLayout")
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2
                                Repeater {
                                    model: ["cmd.singlePage", "cmd.continuous", "cmd.facing", "cmd.continuousFacing"]
                                    delegate: AtlasRadioButton {
                                        required property int index
                                        required property var modelData
                                        text: I18n.t(modelData)
                                        checked: preferences.defaultPageDisplay === index
                                        onClicked: preferences.defaultPageDisplay = index
                                    }
                                }
                            }
                        }
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.defaultZoom")
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2
                                Repeater {
                                    model: ["zoom.fitWidth", "zoom.fitPage", "zoom.actualSize"]
                                    delegate: AtlasRadioButton {
                                        required property int index
                                        required property var modelData
                                        text: I18n.t(modelData)
                                        checked: preferences.defaultZoom === index
                                        onClicked: preferences.defaultZoom = index
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    Layout.topMargin: Theme.sm
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.t("prefs.defaultsHint")
                                    color: Theme.muted
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    // Search
                    ColumnLayout {
                        visible: dialog.page === "search"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.search.title")
                            ColumnLayout {
                                anchors.fill: parent
                                AtlasCheckBox {
                                    text: I18n.t("find.wholeWords")
                                    checked: preferences.searchWholeWords
                                    onToggled: preferences.searchWholeWords = checked
                                }
                                AtlasCheckBox {
                                    text: I18n.t("find.matchCase")
                                    checked: preferences.searchMatchCase
                                    onToggled: preferences.searchMatchCase = checked
                                }
                                Label {
                                    Layout.fillWidth: true
                                    Layout.topMargin: Theme.sm
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.t("prefs.search.arabicHint")
                                    color: Theme.muted
                                    wrapMode: Text.WordWrap
                                }
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
                                    delegate: AtlasRadioButton {
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

                    // Identity (Foxit: Preferences → Identity)
                    ColumnLayout {
                        visible: dialog.page === "identity"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.identity")
                            GridLayout {
                                anchors.fill: parent
                                columns: 2
                                columnSpacing: Theme.md
                                rowSpacing: Theme.sm
                                Label { text: I18n.t("prefs.identity.name"); color: Theme.ink }
                                AtlasTextField {
                                    id: identityName
                                    Layout.fillWidth: true
                                    maximumLength: 256
                                    Accessible.name: I18n.t("prefs.identity.name")
                                    font.family: I18n.contentFamily(text) || I18n.uiFamily
                                }
                                Label { text: I18n.t("prefs.identity.organization"); color: Theme.ink }
                                AtlasTextField {
                                    id: identityOrganization
                                    Layout.fillWidth: true
                                    maximumLength: 256
                                    Accessible.name: I18n.t("prefs.identity.organization")
                                    font.family: I18n.contentFamily(text) || I18n.uiFamily
                                }
                                Label { text: I18n.t("prefs.identity.email"); color: Theme.ink }
                                AtlasTextField {
                                    id: identityEmail
                                    Layout.fillWidth: true
                                    maximumLength: 256
                                    inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
                                    horizontalAlignment: TextInput.AlignLeft
                                    LayoutMirroring.enabled: false   // an address reads left to right
                                    Accessible.name: I18n.t("prefs.identity.email")
                                }
                                Label {
                                    Layout.columnSpan: 2
                                    Layout.fillWidth: true
                                    Layout.topMargin: Theme.sm
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.t("prefs.identity.hint")
                                    color: Theme.muted
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    // Security
                    ColumnLayout {
                        visible: dialog.page === "security"
                        width: pageScroll.availableWidth
                        spacing: Theme.md
                        GroupBox {
                            Layout.fillWidth: true
                            title: I18n.t("prefs.security")
                            ColumnLayout {
                                anchors.fill: parent
                                AtlasCheckBox {
                                    text: I18n.t("prefs.security.recheck")
                                    checked: preferences.recheckOnReturn
                                    onToggled: preferences.recheckOnReturn = checked
                                }
                                AtlasCheckBox {
                                    text: I18n.t("prefs.security.showStatus")
                                    checked: preferences.showDocumentStatus
                                    onToggled: preferences.showDocumentStatus = checked
                                }
                                Label {
                                    Layout.fillWidth: true
                                    Layout.topMargin: Theme.sm
                                    horizontalAlignment: Text.AlignLeft
                                    text: I18n.t("prefs.security.hint")
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
                                    delegate: AtlasRadioButton {
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
                onClicked: dialog.accept()
            }
            AtlasButton {
                implicitWidth: 86
                text: I18n.t("prefs.cancel")
                onClicked: { dialog.restore(); dialog.close() }
            }
        }
    }
}
