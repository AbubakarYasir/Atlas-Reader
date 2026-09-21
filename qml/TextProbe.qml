import QtQuick

Window {
    id: root

    width: 1180
    height: 760
    visible: true
    color: atlasInitialDark ? "#202124" : "#f7f7f8"
    title: "Atlas N1 — text probe"

    Rectangle {
        anchors.fill: parent
        color: root.color

        Text {
            anchors.centerIn: parent
            width: Math.min(parent.width - 64, 820)
            color: atlasInitialDark ? "#f1f3f4" : "#202124"
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: atlasInitialArabic
                ? "هذه مرحلة تشخيصية لقياس تكلفة إنشاء النص العربي وتشكيله وعرضه، من غير تحميل Qt Quick Controls أو التخطيطات الخاصة بالواجهة الكاملة."
                : "This diagnostic profile measures Qt Quick text creation and shaping without loading Qt Quick Controls or the full shell layout."
        }
    }
}
