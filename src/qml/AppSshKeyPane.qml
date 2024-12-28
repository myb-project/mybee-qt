import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Pane {
    id: control
    padding: 0

    property int maxLines: 4
    required property string currentServer
    readonly property string sshKeyFile: (listView.currentIndex >= 0 && listView.currentIndex < listModel.count) ?
                                             listModel.get(listView.currentIndex).text : ""

    Component.onCompleted: {
        //addSshKey(VMConfigSet.cbsdSshKey)
        var list = SystemHelper.sshKeyPairs()
        for (var i = 0; i < list.length; i++) addSshKey(list[i])

        setCurrentIndex()
    }

    onCurrentServerChanged: setCurrentIndex()

    function setCurrentIndex() {
        if (!listModel.count) return
        if (Url.isValidAt(currentServer)) {
            var path = SystemHelper.appDataPath(SystemHelper.fileName(currentServer))
            if (SystemHelper.isDir(path)) {
                var obj = SystemHelper.loadObject(path + "/lastServer")
                if (obj.hasOwnProperty("ssh_key")) {
                    for (var i = 0; i < listModel.count; i++) {
                        if (listModel.get(i).text === obj["ssh_key"]) {
                            listView.currentIndex = i
                            return
                        }
                    }
                }
            }
        }
        listView.currentIndex = 0
    }

    function addSshKey(path) : int {
        if (!path) return -1
        for (var i = 0; i < listModel.count; i++) {
            if (listModel.get(i).text === path) return i
        }
        switch (SystemHelper.isSshKeyPair(path) ? 2 : (SystemHelper.isSshPrivateKey(path) ? 1 : 0)) {
        case 0: listModel.append({ "icon": "qrc:/image-check-bad",  "text": path, "tip": qsTr("This is not an Ssh key") }); break
        case 1: listModel.append({ "icon": "qrc:/image-check-part", "text": path, "tip": qsTr("Private SSH key only") }); break
        case 2: listModel.append({ "icon": "qrc:/image-check-ok",   "text": path, "tip": qsTr("Private & public Ssh keys") }); break
        default: return -1
        }
        return listModel.count - 1
    }

    ListModel {
        id: listModel
    }

    RowLayout {
        anchors.fill: parent
        MyListView {
            id: listView
            Layout.fillWidth: true
            implicitWidth: 250
            implicitHeight: appRowHeight * control.maxLines
            focus: true
            underline: true
            currentIndex: count === 1 ? 0 : -1
            model: listModel
            delegate: ItemDelegate {
                //enabled: !index || Url.schemeAt(control.currentServer) !== "file"
                padding: appTextPadding
                spacing: 0
                width: ListView.view.width
                height: appRowHeight
                highlighted: ListView.isCurrentItem
                text: model.text
                icon.source: model.icon
                icon.color: enabled ? "transparent" : "gray"
                onClicked: {
                    listView.forceActiveFocus()
                    ListView.view.currentIndex = index
                }
                ToolTip.text: model.tip
                ToolTip.visible: hovered
                ToolTip.delay: appTipDelay
                ToolTip.timeout: appTipTimeout
                FlashingPoint {
                    visible: !index && listView.currentIndex < 0
                }
            }
            Label {
                visible: !listView.count
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: appTitleSize
                wrapMode: Text.Wrap
                text: qsTr("Ssh keys not found<br><br>Click the flashing button to find a suitable one")
            }
        }
        Column {
            SquareButton {
                id: addSquareButton
                icon.source: "qrc:/icon-open-folder"
                ToolTip.text: qsTr("Append file")
                onClicked: {
                    var dlg = Qt.createComponent("MyFileDialog.qml").createObject(control, {
                                "title": qsTr("Select Ssh access key") })
                    if (!dlg) { appToast(qsTr("Can't load MyFileDialog.qml")); return }
                    dlg.accepted.connect(function() { listView.currentIndex = control.addSshKey(dlg.filePath()) })
                    dlg.open()
                }
                FlashingPoint { visible: !listView.count }
            }
            SquareButton {
                enabled: control.sshKeyFile
                icon.source: "qrc:/icon-open-view"
                ToolTip.text: qsTr("View file")
                onClicked: appPage("AppFileView.qml", { "filePath": control.sshKeyFile })
            }
            SquareButton {
                enabled: listView.count && ~listView.currentIndex
                icon.source: "qrc:/icon-remove"
                ToolTip.text: qsTr("Remove entry")
                onClicked: listModel.remove(listView.currentIndex)
            }
        }
    }
}
