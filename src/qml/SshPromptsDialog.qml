import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Dialog {
    id: control
    modal: true
    focus: true
    padding: 20
    anchors.centerIn: parent //Overlay.overlay
    width: Math.min(parent.width, 360)
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
    }
    standardButtons: Dialog.Cancel | Dialog.Ok

    property string info
    required property var prompts
    property bool publicKey: false
    readonly property alias shareKey: keyCheckBox.checked

    function answers() {
        var list = []
        for (var i = 0; i < promptsRepeater.count; i++) {
            list.push(promptsRepeater.itemAt(i).answer)
        }
        return list
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: control.padding

        RowLayout {
            Layout.fillWidth: true
            Image {
                Layout.preferredWidth: appIconSize
                Layout.preferredHeight: appIconSize
                fillMode: Image.PreserveAspectFit
                source: "qrc:/image-secret-key"
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: control.info ? control.info : qsTr("The Ssh server asks you the following questions")
            }
        }

        Repeater {
            id: promptsRepeater
            model: control.prompts

            RowLayout {
                property string answer: ""
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: modelData
                }
                MyTextField {
                    id: textField
                    Layout.fillWidth: true
                    Layout.minimumWidth: 150
                    Layout.maximumWidth: 200
                    focus: true
                    placeholderText: qsTr("Enter answer")
                    hide: !visibleButton.checked
                    onEditingFinished: parent.answer = text
                    onAccepted: if (promptsRepeater.count === 1) control.accept()
                }
                MyToolButton {
                    id: visibleButton
                    enabled: textField.text
                    checkable: true
                    icon.source: checked ? "qrc:/icon-eye" : "qrc:/icon-no-eye"
                    ToolTip.text: qsTr("Toggle visibility")
                }
            }
        }

        CheckBox {
            id: keyCheckBox
            visible: control.publicKey
            checked: visible
            text: qsTr("Share public key with this host")
        }
    }
}
