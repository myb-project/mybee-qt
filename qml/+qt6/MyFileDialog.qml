import QtCore
import QtQuick.Dialogs

FileDialog {
    property string path: isMobile ? StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
                                   : StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]

    currentFolder: path.startsWith("file://") ? path : "file://" + path
    options: FileDialog.ReadOnly | FileDialog.HideNameFilterDetails // | FileDialog.DontUseNativeDialog
    onVisibleChanged: if (!visible) destroy()
    function filePath() {
        var str = selectedFile.toString()
        if (!str.startsWith("file:///")) return decodeURIComponent(str)
        var len = (str.charAt(9) === ':' ? 8 : 7)
        return decodeURIComponent(str.substring(len))
    }
}
