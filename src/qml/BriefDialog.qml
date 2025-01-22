import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

MyDialog {
    id: control
    title: titleType[control.type]

    enum Type { Input, Info, Warning, Error }
    property int type: 1
    property alias text: textLabel.text
    property alias placeholderText: textInput.placeholderText
    property alias input: textInput.text
    property alias readOnly: textInput.readOnly

    readonly property var titleType: [ qsTr("Input required"), qsTr("Information"), qsTr("Warning"), qsTr("Error") ]
    readonly property var imageType: [ "qrc:/image-input", "qrc:/image-info", "qrc:/image-warn", "qrc:/image-error" ]

    onAboutToShow: {
        if (input || placeholderText) textInput.forceActiveFocus()
        else if (standardButtons) {
            var button = standardButton(Dialog.Ok)
            if (button !== null)                                        button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Yes)) !== null)    button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Retry)) !== null)  button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Ignore)) !== null) button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Abort)) !== null)  button.forceActiveFocus()
        } else standardButtons = Dialog.Cancel
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
                source: control.imageType[control.type]
            }
            Label {
                id: textLabel
                Layout.fillWidth: true
                visible: text
                wrapMode: Text.Wrap
            }
        }

        RowLayout {
            visible: textInput.text || textInput.placeholderText
            TextArea {
                id: textInput
                Layout.fillWidth: true
                focus: visible
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                //onEditingFinished: if (!readOnly) control.accept()
            }
            MyToolButton {
                enabled: textInput.text
                visible: textInput.readOnly
                icon.source: "qrc:/icon-content-copy"
                ToolTip.text: qsTr("Copy to clipboard")
                onClicked: {
                    SystemHelper.setClipboard(textInput.text)
                    if (textInput.text) appToast(qsTr("Copied to clipboard"))
                }
            }
        }
    }
}
