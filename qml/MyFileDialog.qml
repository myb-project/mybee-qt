import QtQuick 2.15
import QtQuick.Dialogs 1.3

FileDialog {
    property string path: isMobile ? shortcuts.documents : shortcuts.home

    folder: path.startsWith("file://") ? path : "file://" + path
    sidebarVisible: false
    onVisibleChanged: if (!visible) destroy()
    function filePath() {
        var str = fileUrl.toString()
        if (!str.startsWith("file:///")) return decodeURIComponent(str)
        var len = (str.charAt(9) === ':' ? 8 : 7)
        return decodeURIComponent(str.substring(len))
    }
}
