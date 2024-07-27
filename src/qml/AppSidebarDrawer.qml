import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Drawer {
    id: control
    modal: true

    component ItemDelegateTemplate : ItemDelegate {
        width: control.availableWidth
        height: appWindow.header.height
        spacing: appTextPadding
        onClicked: control.close()
    }

    Column {
        anchors.fill: parent
        Pane {
            Material.elevation: 4
            padding: 10
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    Layout.bottomMargin: 10
                    source: "qrc:/image-cloud"
                    text: qsTr("Visit the project homepage")
                    onClicked: {
                        Qt.openUrlExternally("https://www.bsdstore.ru")
                        control.close()
                    }
                }
                Label {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 10
                    horizontalAlignment: Text.AlignHCenter
                    font.pointSize: appTitleSize
                    font.bold: true
                    wrapMode: Text.Wrap
                    text: Qt.application.displayName + "\nv" + Qt.application.version
                }
            }
        }
        ItemDelegateTemplate {
            icon.source: "qrc:/icon-preferences"
            text: qsTr("Preferences")
            onClicked: appPage("PreferencesPage.qml")
        }
        ItemDelegateTemplate {
            icon.source: "qrc:/icon-about"
            text: qsTr("About")
            onClicked: appPage("AppAboutPage.qml")
        }
        MenuSeparator { }
        ItemDelegateTemplate {
            icon.source: "qrc:/icon-quit"
            text: qsTr("Quit")
            onClicked: Qt.quit()
        }
    }
}
