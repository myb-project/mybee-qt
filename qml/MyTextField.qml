import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
    selectByMouse: true
    onActiveFocusChanged: {
        if (!text) return
        if (activeFocus) // && cursorPosition === text.length) {
            selectAll()
        else deselect()
    }
}
