import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

MyDialog {
    id: control
    title: qsTr("Access to %1").arg(urlModel.remote ? urlModel.adjusted(Url.RemoveUserInfo | Url.RemovePath) : urlModel.path)
    standardButtons: Dialog.Cancel | Dialog.Ok

    property alias url: urlModel.location
    property alias scheme: urlModel.scheme
    property alias user: urlModel.userName
    property alias password: urlModel.password
    property alias host: urlModel.host
    property alias port: urlModel.port
    property alias path: urlModel.path

    function userHost() : string {
        return urlModel.userHost()
    }

    onAboutToShow: {
        visibleButton.checked = false
        passwdField.clear()
        if (urlModel.password)
            passwdField.insert(0, urlModel.password)
    }

    UrlModel {
        id: urlModel
        userName: SystemHelper.userName
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
                id: textLabel
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Please enter the password of the <b>%1</b> user to access the <b>%2</b> server")
                    .arg(urlModel.userName)
                    .arg(SystemHelper.camelCase(urlModel.remote ? urlModel.host : SystemHelper.hostName))
            }
        }

        RowLayout {
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Password for %1").arg(urlModel.userName)
            }
            MyTextField {
                id: passwdField
                Layout.fillWidth: true
                Layout.minimumWidth: 150
                Layout.maximumWidth: 200
                focus: true
                placeholderText: qsTr("Enter answer")
                hide: !visibleButton.checked
                onEditingFinished: urlModel.password = text
                onAccepted: control.accept()
            }
            MyToolButton {
                id: visibleButton
                enabled: passwdField.text
                checkable: true
                icon.source: checked ? "qrc:/icon-eye" : "qrc:/icon-no-eye"
                ToolTip.text: qsTr("Toggle visibility")
            }
        }
    }
}
