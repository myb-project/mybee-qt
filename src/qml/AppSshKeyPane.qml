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
        var list = SystemHelper.sshAllKeys()
        for (var i = 0; i < list.length; i++) addSshKey(list[i], false)
    }

    onCurrentServerChanged: {
        if (!Url.isValidAt(currentServer)) return
        var folder = SystemHelper.fileName(currentServer)
        if (!SystemHelper.isDir(folder)) return
        var obj = SystemHelper.loadObject(folder + "/lastServer.json")
        if (SystemHelper.isSshPrivateKey(obj["ssh_key"])) addSshKey(obj["ssh_key"])
    }

    function addSshKey(path, top = true) {
        if (!path) return
        for (var i = 0; i < listModel.count; i++) {
            if (listModel.get(i).text === path) {
                listView.currentIndex = i
                listView.ensureCurrentVisible()
                return
            }
        }
        var obj = { "text": path }
        if (SystemHelper.isSshKeyPair(path)) {
            obj["icon"] = "qrc:/image-check-ok"
            obj["tip"] = qsTr("Private & public Ssh keys")
        } else if (SystemHelper.isSshPrivateKey(path)) {
            obj["icon"] = "qrc:/image-check-part"
            obj["tip"] = qsTr("Private Ssh key only")
        } else {
            obj["icon"] = "qrc:/image-check-bad"
            obj["tip"] = qsTr("This is not an Ssh key")
        }
        if (!listModel.count || !top) {
            listModel.append(obj)
            if (listModel.count !== 1) return
        } else listModel.insert(0, obj)
        listView.currentIndex = 0
        listView.ensureCurrentVisible()
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
            currentIndex: -1
            model: listModel
            delegate: ItemDelegate {
                id: itemDelegate
                padding: appTextPadding
                width: ListView.view.width
                height: appRowHeight
                highlighted: ListView.isCurrentItem
                text: model.text
                contentItem: RowLayout {
                    Image {
                        Layout.preferredWidth: itemDelegate.availableHeight
                        Layout.preferredHeight: itemDelegate.availableHeight
                        fillMode: Image.PreserveAspectFit
                        source: model.icon
                    }
                    Text {
                        Layout.fillWidth: true
                        text: itemDelegate.text
                        font: itemDelegate.font
                        color: itemDelegate.highlighted ? Material.accent : Material.foreground
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideLeft
                    }
                }
                onClicked: {
                    listView.forceActiveFocus()
                    listView.currentIndex = index
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
                icon.source: "qrc:/icon-open-folder"
                ToolTip.text: qsTr("Append file")
                onClicked: {
                    var dlg = Qt.createComponent("MyFileDialog.qml").createObject(control, {
                                "title": qsTr("Select Ssh access key") })
                    if (!dlg) { appToast(qsTr("Can't load MyFileDialog.qml")); return }
                    dlg.accepted.connect(function() { control.addSshKey(dlg.filePath()) })
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
                onClicked: {
                    listModel.remove(listView.currentIndex--)
                    if (listView.currentIndex < 0 && listModel.count) {
                        listView.currentIndex = 0
                        listView.ensureCurrentVisible()
                    }
                }
            }
        }
    }
}
