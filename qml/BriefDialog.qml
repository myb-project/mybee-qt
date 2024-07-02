import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: control
    modal: true
    focus: true
    padding: 20
    anchors.centerIn: parent //Overlay.overlay
    width: Math.min(appWindow.width, 360)
    title: titleType[control.type]
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
    }

    enum Type { Input, Info, Warning, Error }
    property int type: 1
    property alias text: textLabel.text
    property alias input: textField.text
    property alias placeholderText: textField.placeholderText

    readonly property var titleType: [ qsTr("Input"), qsTr("Info"), qsTr("Warning"), qsTr("Error") ]
    readonly property var imageType: [ "qrc:/image-input", "qrc:/image-info", "qrc:/image-warn", "qrc:/image-error" ]

    onAboutToShow: {
        if (input || placeholderText) textField.forceActiveFocus()
        else if (standardButtons) {
            var button = standardButton(Dialog.Ok)
            if (button !== null)                                        button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Yes)) !== null)    button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Retry)) !== null)  button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Ignore)) !== null) button.forceActiveFocus()
            else if ((button = standardButton(Dialog.Abort)) !== null)  button.forceActiveFocus()
        } else standardButtons = Dialog.Cancel
    }

    RowLayout {
        anchors.fill: parent
        Image {
            Layout.preferredWidth: appIconSize
            Layout.preferredHeight: appIconSize
            source: control.imageType[control.type]
            fillMode: Image.PreserveAspectFit
        }
        Label {
            id: textLabel
            Layout.fillWidth: true
            visible: text
            font.pointSize: appTitleSize
            wrapMode: Text.Wrap
        }
        MyTextField {
            id: textField
            Layout.preferredWidth: 200
            visible: text || placeholderText
            focus: visible
            onAccepted: control.accept()
        }
    }
}
