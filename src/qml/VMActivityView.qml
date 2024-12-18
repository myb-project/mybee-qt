import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Item {
    id: control

    readonly property int textPadding: Math.max(Math.ceil(fontMetrics.font.pixelSize * 0.1), 2)
    readonly property int rowHeight: Math.ceil(fontMetrics.height + control.textPadding * 2)

    function appendText(text) {
        var max = 0, list = text.split('\n').filter(Boolean)
        if (list.length > 500) loggerListModel.remove(0, list.length)
        for (var line of list) {
            var w = Math.ceil(fontMetrics.advanceWidth(line) + control.textPadding * 2)
            if (w > max) max = w
            loggerListModel.append({ "text": line })
        }
        if (max > listView.contentWidth) listView.contentWidth = max
        listView.contentHeight = listView.count * control.rowHeight
        listView.positionViewAtEnd()
    }

    FontMetrics {
        id: fontMetrics
        font: appWindow.font
    }

    ListModel {
        id: loggerListModel
    }

    MyListView {
        id: listView
        anchors.fill: parent
        focus: true
        model: loggerListModel
        delegate: Text {
            height: control.rowHeight
            padding: control.textPadding
            font: fontMetrics.font
            text: modelData
            color: modelData.match(/^\d\d:\d\d:\d\d\./) ? Material.accent : Material.foreground
            wrapMode: Text.NoWrap
            verticalAlignment: Text.AlignVCenter
            textFormat: Text.PlainText
        }
        ScrollBar.horizontal: ScrollBar { z: 5; active: listView.count }
        ScrollBar.vertical: ScrollBar { z: 5; active: listView.count }
    }

    Label {
        visible: !loggerListModel.count && !VMConfigSet.configCount
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: appTitleSize
        wrapMode: Text.Wrap
        text: qsTr("<i>Welcome to %1!</i><br><br>Click the flashing button to create your first virtual machine")
                    .arg(Qt.application.displayName)
    }
}
