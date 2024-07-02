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
    title: qsTr("Access for <b>%1</b>").arg(VMConfigSet.cbsdPath)
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
    }

    property alias password: passwdField.text

    signal apply()

    onAboutToShow: {
        visibleButton.checked = false
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
                    text: qsTr("<b>Root</b> is required to use virtual machines on the <i>%1</i>")
                        .arg(SystemHelper.hostName)
                }
            }
        }

        RowLayout {
            Label {
                text: qsTr("Password")
            }
            TextField {
                id: passwdField
                Layout.fillWidth: true
                focus: true
                placeholderText: qsTr("Enter password")
                echoMode: visibleButton.checked ? TextInput.Normal : TextInput.Password
                onActiveFocusChanged: {
                    if (!text) return
                    if (activeFocus && cursorPosition === text.length) selectAll();
                    else deselect()
                }
                onAccepted: {
                    control.apply()
                    control.close()
                }
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
    onApplied: {
        control.apply()
        control.close()
    }
}
