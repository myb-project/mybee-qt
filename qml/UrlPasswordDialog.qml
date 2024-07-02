import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Dialog {
    id: control
    modal: true
    focus: true
    padding: 20
    anchors.centerIn: parent //Overlay.overlay
    width: Math.min(appWindow.width, 360)
    title: qsTr("Authorization")
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
    }

    property alias location: urlModel.location

    signal apply(url location)

    onAboutToShow: {
        visibleButton.checked = false
    }

    UrlModel {
        id: urlModel
        function apply() {
            if (valid && password) {
                control.apply(location)
                control.close()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: control.padding

        Pane {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Image {
                    source: "qrc:/image-secret-key"
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Login to <b>%1@%2</b>").arg(urlModel.userName).arg(urlModel.host)
                }
            }
        }

        RowLayout {
            enabled: urlModel.valid
            Label {
                text: qsTr("Password")
            }
            TextField {
                id: passwdField
                Layout.fillWidth: true
                focus: true
                placeholderText: qsTr("Enter password")
                echoMode: visibleButton.checked ? TextInput.Normal : TextInput.Password
                text: urlModel.password
                onTextEdited: urlModel.password = text
                onActiveFocusChanged: {
                    if (!text) return
                    if (activeFocus && cursorPosition === text.length) selectAll();
                    else deselect()
                }
                onAccepted: urlModel.apply()
            }
            ToolButton {
                id: visibleButton
                focusPolicy: Qt.NoFocus
                enabled: passwdField.text
                checkable: true
                icon.source: checked ? "qrc:/icon-eye" : "qrc:/icon-no-eye"
            }
        }
    }

    standardButtons: Dialog.Cancel | Dialog.Apply
    onApplied: urlModel.apply()
}
