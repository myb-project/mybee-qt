import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Pane {
    id: control
    padding: 0

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    property var authConfig: ({})
    property bool modified: false
    readonly property bool isValid: authConfig.hasOwnProperty(RestApiSet.sshPrivName)
    readonly property string appSshKey: SystemHelper.appSshKey

    Component.onCompleted: {
        dropDownView.setIndexEnable(0, true)
    }

    readonly property var dropDownModel: [
        {   // index 0
            icon: "",
            text: QT_TR_NOOP("SSH keys"),
            list: [ { text: QT_TR_NOOP("Active key"), item: activeKeyComponent },
                    { text: QT_TR_NOOP("Found keys"), item: foundKeysComponent },
                    { text: QT_TR_NOOP("Select from"), item: selectKeyComponent } ]
        }
    ]

    function setVarValue(varName, value) {
        //console.debug("setVarValue", varName, value)
        if (authConfig[varName] !== value) {
            var obj = authConfig
            if (value !== undefined && value !== "") obj[varName] = value
            else if (obj.hasOwnProperty(varName)) delete obj[varName]
            else return
            authConfig = obj
            modified = true
        }
    }

    // SSH keys

    Component {
        id: activeKeyComponent
        MyTextField {
            readOnly: true
            text: authConfig.hasOwnProperty(RestApiSet.sshPrivName) ? authConfig[RestApiSet.sshPrivName] : ""
        }
    }

    Component {
        id: foundKeysComponent
        RowLayout {
            ComboBox {
                Layout.fillWidth: true
                selectTextByMouse: true
                editable: true
                //focus: !isMobile
                model: authKeyList
                onActivated: setVarValue(RestApiSet.sshPrivName, currentText)
                onAccepted: setVarValue(RestApiSet.sshPrivName, editText)
                onActiveFocusChanged: if (!activeFocus) setVarValue(RestApiSet.sshPrivName, editText)
                Component.onCompleted: {
                    if (authConfig.hasOwnProperty(RestApiSet.sshPrivName)) {
                        editText = authConfig[RestApiSet.sshPrivName]
                    } else if (control.appSshKey) {
                        setVarValue(RestApiSet.sshPrivName, control.appSshKey)
                    } else if (currentText) {
                        setVarValue(RestApiSet.sshPrivName, currentText)
                    }
                }
            }
            RoundButton {
                focusPolicy: Qt.NoFocus
                icon.source: "qrc:/icon-open-folder"
                onClicked: {
                    var dlg = Qt.createComponent("MyFileDialog.qml").createObject(control, {
                                title: qsTr("Select SSH private key") })
                    if (!dlg) { appToast(qsTr("Can't load MyFileDialog.qml")); return }
                    dlg.accepted.connect(function() {
                        var path = dlg.filePath()
                        if (SystemHelper.isSshKeyPair(path)) setVarValue(RestApiSet.sshPrivName, path)
                        else appInfo(qsTr("There is no valid SSH key"))
                    })
                    dlg.open()
                }
            }
        }
    }

    Component {
        id: selectKeyComponent
        RowLayout {
            readonly property bool useAppKey: authConfig[RestApiSet.sshPrivName] === control.appSshKey
            RadioButton {
                enabled: authKeyList.length
                text: qsTr("Found keys")
                checked: !parent.useAppKey
                onToggled: {
                    if (checked && authKeyList.length)
                        setVarValue(RestApiSet.sshPrivName, authKeyList[0])
                }
            }
            RadioButton {
                enabled: control.appSshKey
                text: qsTr("Application key")
                checked: parent.useAppKey
                onToggled: {
                    if (checked && control.appSshKey)
                        setVarValue(RestApiSet.sshPrivName, control.appSshKey)
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    DropDownView {
        id: dropDownView
        anchors.fill: parent
        myClassName: control.myClassName
        model: control.dropDownModel
    }
}
