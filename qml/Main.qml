import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root

    property bool arabic: atlasInitialArabic
    property bool darkMode: atlasInitialDark

    width: 1180
    height: 760
    minimumWidth: 760
    minimumHeight: 520
    visible: true
    title: arabic ? "أطلس ريدر — خط الأساس N1" : "Atlas Reader Native — N1 Baseline"

    LayoutMirroring.enabled: arabic
    LayoutMirroring.childrenInherit: true

    palette.window: darkMode ? "#202124" : "#f7f7f8"
    palette.windowText: darkMode ? "#f1f3f4" : "#202124"
    palette.base: darkMode ? "#292a2d" : "#ffffff"
    palette.text: darkMode ? "#f1f3f4" : "#202124"
    palette.button: darkMode ? "#303134" : "#eceff1"
    palette.buttonText: darkMode ? "#f1f3f4" : "#202124"
    palette.highlight: darkMode ? "#8ab4f8" : "#3367d6"
    palette.highlightedText: darkMode ? "#202124" : "#ffffff"

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10

            Label {
                text: root.arabic ? "أطلس ريدر" : "Atlas Reader Native"
                font.pixelSize: 18
                font.bold: true
            }

            Label {
                text: atlasVersion
                opacity: 0.65
            }

            Item { Layout.fillWidth: true }

            Button {
                text: root.arabic ? "English" : "العربية"
                Accessible.name: root.arabic ? "Switch to English" : "التبديل إلى العربية"
                onClicked: root.arabic = !root.arabic

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        if (!event.isAutoRepeat)
                            animateClick()
                        event.accepted = true
                    }
                }
            }

            Button {
                text: root.darkMode
                    ? (root.arabic ? "الوضع الفاتح" : "Light")
                    : (root.arabic ? "الوضع الداكن" : "Dark")
                Accessible.name: root.darkMode
                    ? (root.arabic ? "استخدام الوضع الفاتح" : "Use light theme")
                    : (root.arabic ? "استخدام الوضع الداكن" : "Use dark theme")
                onClicked: root.darkMode = !root.darkMode

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        if (!event.isAutoRepeat)
                            animateClick()
                        event.accepted = true
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 820)
        spacing: 18

        Label {
            Layout.fillWidth: true
            text: root.arabic ? "خط أساس ويندوز الأصلي" : "Native Windows baseline"
            font.pixelSize: 32
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            text: root.arabic
                ? "هذه مرحلة N1 لقياس تكلفة الواجهة الفارغة، والتحقق من اتجاه العربية، والوضعين الفاتح والداكن، ودورة التشغيل والإغلاق قبل إضافة محرك PDF أو الفهرسة."
                : "N1 measures the empty-shell cost, validates Arabic direction, light/dark presentation, and clean startup/shutdown before any PDF engine or indexing code is added."
            font.pixelSize: 16
            opacity: 0.82
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Frame {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    text: root.arabic ? "ترتيب المنتج الثابت" : "Locked product order"
                    font.bold: true
                }

                Label { text: root.arabic ? "١. المكتبة / الفهرس" : "1. Library / Index" }
                Label { text: root.arabic ? "٢. القارئ" : "2. Reader" }
                Label { text: root.arabic ? "٣. العلامات / الفهارس" : "3. Bookmarks / Outlines" }
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.arabic
                ? "لا توجد في N1 أي ميزات PDF أو قاعدة بيانات أو علامات. هذا مقصود."
                : "N1 intentionally contains no PDF, database, or bookmark product features."
            opacity: 0.62
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
}
