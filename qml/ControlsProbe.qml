import QtQuick
import QtQuick.Controls.Basic

ApplicationWindow {
    id: root

    width: 1180
    height: 760
    visible: true
    title: "Atlas N1 — Basic Controls probe"

    palette.window: atlasInitialDark ? "#202124" : "#f7f7f8"
    palette.windowText: atlasInitialDark ? "#f1f3f4" : "#202124"
    palette.button: atlasInitialDark ? "#303134" : "#eceff1"
    palette.buttonText: atlasInitialDark ? "#f1f3f4" : "#202124"

    Label {
        id: titleLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 220
        width: Math.min(parent.width - 64, 820)
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pixelSize: 22
        font.bold: true
        text: atlasInitialArabic
            ? "مسبار Qt Quick Controls Basic"
            : "Qt Quick Controls Basic probe"
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: titleLabel.bottom
        anchors.topMargin: 24
        width: Math.min(parent.width - 64, 820)
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: atlasInitialArabic
            ? "يقيس هذا الملف تكلفة تحميل عناصر Label وButton من النمط Basic من غير تخطيط الواجهة الكاملة."
            : "This profile measures Basic Label and Button loading without the full application shell composition."
    }

    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: titleLabel.bottom
        anchors.topMargin: 110
        text: atlasInitialArabic ? "زر تجريبي" : "Probe button"
    }
}
