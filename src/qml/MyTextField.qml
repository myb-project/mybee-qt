import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
    property bool hide: false
    echoMode: hide ? TextInput.Password : TextInput.Normal
    selectByMouse: true
    onActiveFocusChanged: {
        if (!text) return
        if (activeFocus) // && cursorPosition === text.length) {
            selectAll()
        else deselect()
    }
}
