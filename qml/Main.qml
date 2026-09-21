import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    minimumWidth: 760
    minimumHeight: 520
    visible: true
    title: qsTr("Atlas Reader Native — Bootstrap")

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Label {
                text: qsTr("Atlas Reader Native")
                font.pixelSize: 18
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Label {
                text: qsTr("N0 · Architecture bootstrap")
                opacity: 0.7
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 760)
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: qsTr("Native foundation ready for review")
            font.pixelSize: 30
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("This build intentionally contains no PDF reader, index, or bookmark implementation yet. The repository is establishing the C++23 + Qt Quick architecture and its contracts first.")
            font.pixelSize: 16
            opacity: 0.78
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Frame {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label { text: qsTr("1. Index / Library") }
                Label { text: qsTr("2. Reader") }
                Label { text: qsTr("3. Bookmarks / Outlines") }
            }
        }
    }
}
