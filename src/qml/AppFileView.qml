import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Page {
    id: control
    title: filePath

    required property string filePath
    property int maxFileSize: 10 * 1024 // ~10KB

    Component.onCompleted: {
        var size = SystemHelper.fileSize(filePath)
        if (size > maxFileSize) appInfo(qsTr("The file is too large (max %1KB)").arg(maxFileSize / 1024))
        else listModel.refresh(SystemHelper.loadText(filePath))
    }

    FontMetrics {
        id: fontMetrics
        font: control.font
    }

    ListModel {
        id: listModel
        function refresh(list) {
            var max = 0
            for (var line of list) {
                var w = Math.ceil(fontMetrics.advanceWidth(line) + appTextPadding * 2)
                if (w > max) max = w
                append({ "text": line })
            }
            if (count) {
                if (max > listView.contentWidth) listView.contentWidth = max
                listView.contentHeight = count * appRowHeight
                listView.positionViewAtEnd()
            } else {
                listView.contentWidth = 0
                listView.contentHeight = 0
                listView.positionViewAtBeginning()
            }
        }
    }

    MyListView {
        id: listView
        anchors.fill: parent
        focus: true

        model: listModel
        delegate: Text {
            height: appRowHeight
            padding: appTextPadding
            font: control.font
            text: modelData
            color: Material.foreground
            wrapMode: Text.NoWrap
            verticalAlignment: Text.AlignVCenter
        }

        ScrollBar.horizontal: ScrollBar { z: 2; active: listView.count }
        ScrollBar.vertical: ScrollBar { z: 3; active: listView.count }
    }
}
