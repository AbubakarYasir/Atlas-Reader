import QtQuick

// Reader: one workspace per open document; the tabs themselves live in the
// shared DocumentTabBar (Library is the first tab, as Foxit's Start tab).
Item {
    id: reader

    readonly property var currentTab: tabStack.currentTab

    signal linkConfirmRequested(string url)
    signal toastRequested(string text)

    function cycleTab(step) {
        const count = tabRepeater.count
        if (count === 0) return
        const current = readerController.activeIndex
        readerController.activeIndex = current < 0 ? 0 : (current + step + count) % count
    }

    Item {
        id: tabStack
        anchors.fill: parent
        property var currentTab: null

        Repeater {
            id: tabRepeater
            model: readerController
            delegate: ReaderTab {
                anchors.fill: parent
                visible: index === readerController.activeIndex
                onVisibleChanged: if (visible) tabStack.currentTab = this
                Component.onCompleted: {
                    if (visible) tabStack.currentTab = this
                    if (ready) preferences.addRecentFile(sourcePath)
                }
                onReadyChanged: if (ready) preferences.addRecentFile(sourcePath)
                onLinkConfirmRequested: (url) => reader.linkConfirmRequested(url)
                onToastRequested: (text) => reader.toastRequested(text)
            }
        }
    }
}
