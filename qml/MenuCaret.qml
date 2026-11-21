import QtQuick

// Small solid down-pointing triangle that marks a button with a menu.
// Drawn, not typed: the "▾" character is missing from the Windows interface
// font, and one missing character makes Qt read every installed font.
Item {
    id: caret

    property color color: Theme.ink

    implicitWidth: 7
    implicitHeight: 4
    clip: true

    Rectangle {
        width: 5
        height: 5
        x: 1
        y: -2.5
        rotation: 45
        antialiasing: true
        color: caret.color
    }
}
