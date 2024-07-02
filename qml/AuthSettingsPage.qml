import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("Authorization")

    property string currentServer
    property var serverList: SystemHelper.fileList(null, null, true)

    // used by inner items
    property var authKeyList: SystemHelper.sshKeyPairs()

    Component.onCompleted: {
        if (currentServer) {
            var index = serverList.indexOf(SystemHelper.fileName(currentServer))
            if (~index) tabBar.setCurrentIndex(index)
        }
    }

    header: Column {
        Pane {
            width: parent.width
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-ssh-keys"
                    text: qsTr("Visit Wikipedia")
                    onClicked: Qt.openUrlExternally("https://wikipedia.org/wiki/Secure_Shell")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Settings are required to manage a virtual machine on a cloud server and launch a remote terminal")
                }
            }
        }
        TabBar {
            id: tabBar
            width: parent.width
            currentIndex: swipeView.currentIndex
            Repeater {
                model: control.serverList
                TabButton {
                    rightPadding: spacing + removeTabButton.width
                    width: implicitWidth
                    text: modelData
                    ToolButton {
                        id: removeTabButton
                        anchors.right: parent.right
                        focusPolicy: Qt.NoFocus
                        enabled: SystemHelper.isFile(parent.text + '/' + RestApiSet.authConfFile)
                        icon.source: "qrc:/icon-close"
                        highlighted: tabBar.currentItem == parent
                        onClicked: {
                            appWarning(qsTr("Do you want to remove SSH config at %1?").arg(parent.text),
                                       Dialog.Yes | Dialog.No).accepted.connect(function() {
                                enabled = !SystemHelper.removeFile(parent.text + '/' + RestApiSet.authConfFile)
                            })
                        }
                    }
                }
            }
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        focus: true // turn-on active focus here
        contentItem.focus: true // propagate to children items
        Repeater {
            model: control.serverList
            AuthConfigPane {
                authConfig: SystemHelper.isFile(modelData + '/' + RestApiSet.authConfFile)
                            ? SystemHelper.loadObject(modelData + '/' + RestApiSet.authConfFile)
                            : SystemHelper.loadObject(RestApiSet.authConfFile)
            }
        }
    }

    footer: DropDownPane {
        show: swipeView.currentItem && swipeView.currentItem.modified
        enabled: show && swipeView.currentItem.isValid
        RowLayout {
            anchors.fill: parent
            CheckBox {
                id: defaultCheckBox
                Layout.fillWidth: true
                checked: !SystemHelper.isFile(RestApiSet.authConfFile)
                text: qsTr("Make it default")
            }
            DialogButtonBox {
                enabled: swipeView.currentItem
                position: DialogButtonBox.Footer
                standardButtons: DialogButtonBox.Ok
                onAccepted: {
                    var config = swipeView.currentItem.authConfig ? swipeView.currentItem.authConfig : {}
                    var path = config[RestApiSet.sshPrivName]
                    if (!path || !SystemHelper.isSshKeyPair(path)) {
                        appError(qsTr("No valid SSH key pair provided"))
                        return
                    }
                    if (!config[RestApiSet.sshPubName]) config[RestApiSet.sshPubName] = path + ".pub"

                    path = defaultCheckBox.checked ? RestApiSet.authConfFile :
                                control.serverList[swipeView.currentIndex] + '/' + RestApiSet.authConfFile
                    if (!SystemHelper.saveObject(path, config)) {
                        appError(qsTr("Can't save the current configuration"))
                        return
                    }
                    VMConfigSet.getList(control.serverList[swipeView.currentIndex])
                    appStackView.pop()
                }
            }
        }
    }
}
