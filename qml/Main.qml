import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root

    property bool arabic: atlasInitialArabic
    property bool darkMode: atlasInitialDark
    property bool gridMode: false
    property color canvas: darkMode ? "#17191d" : "#f5f3ed"
    property color panel: darkMode ? "#20242a" : "#fffdf8"
    property color raised: darkMode ? "#292e36" : "#ffffff"
    property color ink: darkMode ? "#f4f2eb" : "#20231f"
    property color muted: darkMode ? "#aeb5bf" : "#667064"
    property color line: darkMode ? "#3a414b" : "#d9ddd4"
    property color accent: darkMode ? "#9fc8a7" : "#28633a"
    property color accentSoft: darkMode ? "#243d2c" : "#e3f0e5"
    property string errorText: ""

    width: 1260
    height: 800
    minimumWidth: 760
    minimumHeight: 560
    visible: true
    title: arabic ? "أطلس ريدر — المكتبة" : "Atlas Reader — Library"
    color: canvas

    palette.window: canvas
    palette.windowText: ink
    palette.base: raised
    palette.text: ink
    palette.button: darkMode ? "#30353d" : "#eef0eb"
    palette.buttonText: ink
    palette.highlight: accent
    palette.highlightedText: darkMode ? "#172019" : "#ffffff"
    palette.placeholderText: muted

    LayoutMirroring.enabled: arabic
    LayoutMirroring.childrenInherit: true

    function statusText(value) {
        if (value === "scanning") return arabic ? "جارٍ فحص المكتبة دون إيقاف الواجهة…" : "Scanning in the background…"
        if (value === "complete") return arabic
            ? "اكتمل الفحص — أضيف " + libraryController.lastAddedCount + " كتاب"
            : "Scan complete — " + libraryController.lastAddedCount + " added"
        if (value === "completeWithErrors") return arabic ? "اكتمل الفحص مع ملفات تحتاج مراجعة" : "Scan complete; some files need attention"
        if (value === "noRoots") return arabic ? "أضف مجلد كتب للبدء" : "Add a book folder to begin"
        if (value === "error") return arabic ? "تعذر إكمال العملية" : "The operation could not be completed"
        return arabic ? "المكتبة جاهزة" : "Library ready"
    }

    function availabilityText(value) {
        if (value === "available") return arabic ? "متاح" : "Available"
        if (value === "locked") return arabic ? "مقفل بكلمة مرور" : "Password locked"
        if (value === "offline") return arabic ? "المجلد غير متصل" : "Folder offline"
        if (value === "missing") return arabic ? "الملف مفقود" : "Missing"
        if (value === "unreadable") return arabic ? "غير قابل للقراءة" : "Unreadable"
        if (value === "unsupported") return arabic ? "غير مدعوم" : "Unsupported"
        if (value === "readOnly") return arabic ? "للقراءة فقط" : "Read only"
        return arabic ? "غير متاح" : "Unavailable"
    }

    Shortcut { sequence: "Ctrl+K"; onActivated: searchField.forceActiveFocus() }
    Shortcut { sequence: "Ctrl+O"; onActivated: folderDialog.open() }
    Shortcut { sequence: "F5"; onActivated: libraryController.rescan() }

    FolderDialog {
        id: folderDialog
        title: root.arabic ? "اختر مجلد الكتب" : "Choose a book folder"
        onAccepted: libraryController.addRoot(selectedFolder)
    }

    Connections {
        target: libraryController
        function onOperationError(message) {
            console.warn("Atlas operation failed:", message)
            root.errorText = root.arabic
                ? "تعذر إكمال هذا الإجراء. لم تتغير مكتبتك."
                : "Atlas couldn’t complete that action. Your library was not changed."
            errorPopup.open()
        }
    }

    Popup {
        id: errorPopup
        x: Math.max(20, root.width - width - 24)
        y: root.header.height + 18
        width: Math.min(440, root.width - 40)
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: root.darkMode ? "#4a2525" : "#fff0ee"; radius: 8; border.color: "#b54a45" }
        contentItem: Label {
            text: root.errorText
            color: root.darkMode ? "#ffd7d3" : "#7a201c"
            wrapMode: Text.WordWrap
            padding: 12
        }
    }

    Popup {
        id: reviewPopup
        anchors.centerIn: parent
        width: Math.min(700, root.width - 48)
        height: Math.min(560, root.height - 72)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: root.panel; radius: 10; border.color: root.line }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                Label { text: root.arabic ? "مراجعة تغييرات الملفات" : "Review file changes"; color: root.ink; font.pixelSize: 22; font.bold: true }
                Item { Layout.fillWidth: true }
                AtlasButton { text: "×"; flat: true; Accessible.name: root.arabic ? "إغلاق" : "Close"; onClicked: reviewPopup.close() }
            }

            Label {
                Layout.fillWidth: true
                text: root.arabic
                    ? "لا يغيّر أطلس هوية الكتاب عند الشك. وافق فقط على النقل الذي تعرفه."
                    : "Atlas never guesses a book’s identity. Apply only a move you recognize."
                color: root.muted
                wrapMode: Text.WordWrap
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 10
                model: libraryController.pendingProposals

                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: proposalColumn.implicitHeight + 24
                    radius: 8
                    color: root.raised
                    border.color: root.line

                    ColumnLayout {
                        id: proposalColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 7

                        Label {
                            text: modelData.canApply
                                ? (root.arabic ? "نقل محتمل موثوق" : "Verified move candidate")
                                : (root.arabic ? "تغيير غامض — لا يُطبّق تلقائياً" : "Ambiguous change — never automatic")
                            color: modelData.canApply ? root.accent : (root.darkMode ? "#efc27e" : "#87550b")
                            font.bold: true
                        }
                        Label { Layout.fillWidth: true; text: (root.arabic ? "من: " : "From: ") + modelData.beforePath; color: root.muted; elide: Text.ElideMiddle }
                        Label { Layout.fillWidth: true; text: (root.arabic ? "إلى: " : "To: ") + modelData.candidatePath; color: root.ink; elide: Text.ElideMiddle }
                        RowLayout {
                            Layout.alignment: Qt.AlignRight
                            AtlasButton {
                                text: root.arabic ? "تجاهل" : "Dismiss"
                                onClicked: libraryController.dismissProposal(modelData.id)
                            }
                            AtlasButton {
                                visible: modelData.canApply
                                text: root.arabic ? "اعتماد النقل" : "Apply move"
                                highlighted: true
                                onClicked: libraryController.applyProposal(modelData.id)
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: libraryController.pendingCount === 0
                    text: root.arabic ? "لا توجد تغييرات تنتظر المراجعة" : "No file changes need review"
                    color: root.muted
                }
            }
        }
    }

    header: ToolBar {
        implicitHeight: 62
        background: Rectangle { color: root.panel; border.color: root.line }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            spacing: 12

            Rectangle {
                width: 34; height: 34; radius: 8; color: root.accent
                Label { anchors.centerIn: parent; text: "A"; color: "white"; font.bold: true; font.pixelSize: 18 }
            }
            ColumnLayout {
                spacing: 0
                Layout.preferredWidth: 190
                Label { text: root.arabic ? "أطلس ريدر" : "Atlas Reader"; color: root.ink; font.bold: true; font.pixelSize: 17 }
                Label { text: root.arabic ? "مكتبتك البحثية" : "Research library"; color: root.muted; font.pixelSize: 11 }
            }
            Item { Layout.fillWidth: true }
            AtlasButton {
                text: root.arabic ? "English" : "العربية"
                Accessible.name: root.arabic ? "Switch to English" : "التبديل إلى العربية"
                onClicked: root.arabic = !root.arabic
            }
            AtlasButton {
                text: root.darkMode ? "☀" : "☾"
                Accessible.name: root.darkMode ? (root.arabic ? "الوضع الفاتح" : "Light theme") : (root.arabic ? "الوضع الداكن" : "Dark theme")
                onClicked: root.darkMode = !root.darkMode
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: root.width < 900 ? 190 : 230
            Layout.fillHeight: true
            color: root.panel
            border.color: root.line

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 7

                Label { text: root.arabic ? "المكتبة" : "LIBRARY"; color: root.muted; font.bold: true; font.pixelSize: 11; leftPadding: 10 }
                AtlasButton {
                    Layout.fillWidth: true
                    text: root.arabic ? "كل الكتب" : "All books"
                    highlighted: libraryController.viewMode === 0 && libraryController.selectedRootId === ""
                    onClicked: { libraryController.selectedRootId = ""; libraryController.viewMode = 0 }
                }
                AtlasButton {
                    Layout.fillWidth: true
                    text: root.arabic ? "المفضلة" : "Favorites"
                    highlighted: libraryController.viewMode === 1
                    onClicked: libraryController.viewMode = 1
                }
                AtlasButton {
                    Layout.fillWidth: true
                    text: root.arabic ? "المستخدمة مؤخراً" : "Recently opened"
                    highlighted: libraryController.viewMode === 2
                    onClicked: libraryController.viewMode = 2
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: root.line; Layout.topMargin: 8; Layout.bottomMargin: 8 }
                Label { text: root.arabic ? "المجلدات" : "FOLDERS"; color: root.muted; font.bold: true; font.pixelSize: 11; leftPadding: 10 }
                Repeater {
                    model: libraryController.roots
                    delegate: AtlasButton {
                        required property var modelData
                        Layout.fillWidth: true
                        text: (modelData.availability === "available" ? "●  " : "○  ")
                            + modelData.name
                            + (modelData.availability === "available"
                                ? ""
                                : (root.arabic ? " — غير متصل" : " — offline"))
                        highlighted: libraryController.selectedRootId === modelData.id
                        Accessible.description: modelData.path
                        onClicked: libraryController.selectedRootId = modelData.id
                    }
                }
                AtlasButton {
                    Layout.fillWidth: true
                    text: root.arabic ? "+ إضافة مجلد" : "+ Add folder"
                    Accessible.name: root.arabic ? "إضافة مجلد كتب، الاختصار كنترول أو" : "Add book folder, Ctrl+O"
                    onClicked: folderDialog.open()
                }

                Item { Layout.fillHeight: true }
                AtlasButton {
                    Layout.fillWidth: true
                    visible: libraryController.pendingCount > 0
                    text: root.arabic
                        ? "مراجعة التغييرات (" + libraryController.pendingCount + ")"
                        : "Review changes (" + libraryController.pendingCount + ")"
                    onClicked: reviewPopup.open()
                }
                Label {
                    Layout.fillWidth: true
                    text: atlasVersion + " · N3"
                    color: root.muted
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 22
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 360
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        Layout.minimumHeight: implicitHeight
                        text: libraryController.viewMode === 1
                            ? (root.arabic ? "المفضلة" : "Favorites")
                            : libraryController.viewMode === 2
                                ? (root.arabic ? "المستخدمة مؤخراً" : "Recently opened")
                                : libraryController.selectedRootName !== ""
                                    ? libraryController.selectedRootName
                                    : (root.arabic ? "مكتبتي" : "My library")
                        color: root.ink
                        font.pixelSize: 28
                        font.bold: true
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.minimumHeight: implicitHeight
                        text: root.arabic ? "ابحث عن الكتاب الصحيح واحتفظ بهويته أينما نُقل" : "Find the right book and keep its identity wherever it moves"
                        color: root.muted
                    }
                }
                AtlasButton {
                    text: root.arabic ? "فحص الآن" : "Rescan"
                    enabled: !libraryController.scanning && libraryController.rootCount > 0
                    Accessible.name: root.arabic ? "إعادة فحص جميع المجلدات، إف 5" : "Rescan all folders, F5"
                    onClicked: libraryController.rescan()
                }
                AtlasButton {
                    text: root.arabic ? "إضافة مجلد" : "Add folder"
                    highlighted: true
                    onClicked: folderDialog.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: root.arabic ? "ابحث بالعنوان أو المؤلف أو اسم الملف…" : "Search title, author, or filename…"
                    Accessible.name: root.arabic ? "بحث المكتبة" : "Search library"
                    selectByMouse: true
                    onTextEdited: searchDelay.restart()
                    Keys.onEscapePressed: { text = ""; libraryController.query = "" }
                    Timer { id: searchDelay; interval: 180; onTriggered: libraryController.query = searchField.text }
                }
                AtlasButton { text: root.arabic ? "قائمة" : "List"; highlighted: !root.gridMode; Accessible.name: root.arabic ? "عرض القائمة" : "List view"; onClicked: root.gridMode = false }
                AtlasButton { text: root.arabic ? "شبكة" : "Grid"; highlighted: root.gridMode; Accessible.name: root.arabic ? "عرض الشبكة" : "Grid view"; onClicked: root.gridMode = true }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 38
                radius: 7
                color: libraryController.statusKey === "error" ? (root.darkMode ? "#4a2525" : "#fff0ee") : root.accentSoft
                border.color: libraryController.statusKey === "error" ? "#b54a45" : root.accent
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    BusyIndicator { running: libraryController.scanning; visible: running; implicitWidth: 22; implicitHeight: 22 }
                    Label { Layout.fillWidth: true; text: root.statusText(libraryController.statusKey); color: root.ink; elide: Text.ElideRight }
                    AtlasButton {
                        visible: libraryController.pendingCount > 0
                        flat: true
                        text: root.arabic ? "مراجعة" : "Review"
                        onClicked: reviewPopup.open()
                    }
                }
            }

            GridView {
                id: bookView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: libraryController
                cellWidth: root.gridMode ? Math.max(240, width / Math.max(1, Math.floor(width / 280))) : width
                cellHeight: root.gridMode ? 206 : 132
                keyNavigationEnabled: true
                activeFocusOnTab: true

                delegate: Item {
                    required property string documentId
                    required property string bookTitle
                    required property string bookAuthor
                    required property string sourcePath
                    required property string fileName
                    required property string availability
                    required property bool favorite
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 6
                        radius: 9
                        color: root.raised
                        border.color: parent.GridView.isCurrentItem ? root.accent : root.line
                        border.width: parent.GridView.isCurrentItem ? 2 : 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 13

                            Rectangle {
                                Layout.preferredWidth: root.gridMode ? 58 : 52
                                Layout.preferredHeight: root.gridMode ? 80 : 70
                                radius: 5
                                color: availability === "available" ? root.accentSoft : (root.darkMode ? "#3b3330" : "#f3e8dc")
                                Label {
                                    anchors.centerIn: parent
                                    text: "PDF"
                                    color: availability === "available" ? root.accent : root.muted
                                    font.bold: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 4
                                Label {
                                    Layout.fillWidth: true
                                    text: bookTitle || fileName
                                    color: root.ink
                                    font.pixelSize: 16
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: bookAuthor || (root.arabic ? "مؤلف غير مسجل" : "Unknown author")
                                    color: root.muted
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: sourcePath
                                    color: root.muted
                                    font.pixelSize: 11
                                    elide: Text.ElideMiddle
                                }
                                Item { Layout.fillHeight: true }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        text: root.availabilityText(availability)
                                        color: availability === "available" ? root.accent : (root.darkMode ? "#efc27e" : "#87550b")
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                    Item { Layout.fillWidth: true }
                                    AtlasButton {
                                        text: favorite ? "★" : "☆"
                                        flat: true
                                        Accessible.name: favorite
                                            ? (root.arabic ? "إزالة من المفضلة" : "Remove from favorites")
                                            : (root.arabic ? "إضافة إلى المفضلة" : "Add to favorites")
                                        onClicked: libraryController.setFavorite(documentId, !favorite)
                                    }
                                    AtlasButton {
                                        text: root.arabic ? "فتح" : "Open"
                                        enabled: availability === "available" || availability === "readOnly"
                                        opacity: enabled ? 1.0 : 0.42
                                        Accessible.description: root.arabic ? "يفتح في قارئ ويندوز حالياً؛ قارئ أطلس يأتي في N4" : "Opens in the Windows PDF app for now; the Atlas reader arrives in N4"
                                        onClicked: libraryController.openExternally(documentId)
                                    }
                                }
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    width: Math.min(500, parent.width - 40)
                    visible: bookView.count === 0
                    text: searchField.text.length > 0
                        ? (root.arabic ? "لا توجد كتب مطابقة. جرّب كلمة من العنوان أو اسم الملف." : "No matching books. Try a word from the title or filename.")
                        : libraryController.rootCount === 0
                            ? (root.arabic ? "ابدأ بإضافة مجلد كتب. سيبقى الفحص في الخلفية ولن يحذف الكتب عند فصل القرص." : "Add a book folder to begin. Scanning stays in the background and disconnected drives never erase your library.")
                            : (root.arabic ? "لا توجد كتب في هذا العرض بعد." : "No books in this view yet.")
                    color: root.muted
                    font.pixelSize: 16
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
