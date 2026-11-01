pragma Singleton
import QtQuick

// Window-level UI state shared by views (not persisted).
QtObject {
    property bool readMode: false
    property bool fullScreen: false
    readonly property bool chromeHidden: readMode || fullScreen
}
