import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Drawer {
    id: control
    modal: true
    enabled: !VMConfigSet.isBusy

    property var configMap: ({})

    onAboutToShow: {
        control.configMap = VMConfigSet.configMap()
    }

    readonly property list<Action> actionModel: [
        Action {
            icon.source: "qrc:/icon-preferences"
            text: qsTr("Preferences")
            onTriggered: appPage("PreferencesPage.qml")
        },
        Action {
            enabled: listView.count
            icon.source: "qrc:/icon-ssh"
            text: qsTr("Authorization")
            //shortcut: "Ctrl+A"
            onTriggered: appPage("AuthSettingsPage.qml")
        },
        Action {
            icon.source: "qrc:/icon-about"
            text: qsTr("About")
            onTriggered: appPage("AppAboutPage.qml")
        },
        Action {
            icon.source: "qrc:/icon-quit"
            text: qsTr("Quit")
            shortcut: StandardKey.Quit
            //onTriggered: appWindow.close()
            onTriggered: Qt.quit()
        }
    ]

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        focus: !isMobile
        header: Pane {
            Material.elevation: 4
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-box"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://www.bsdstore.ru")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Virtual machines")
                }
            }
        }
        model: Object.keys(control.configMap)

        delegate: ItemDelegate {
            width: listView.width
            padding: appTextPadding
            spacing: padding
            text: control.configMap[modelData]["alias"]
            highlighted: text === VMConfigSet.valueAt("alias")
            contentItem: Text {
                padding: parent.padding
                font: parent.font
                text: parent.text
                color: parent.highlighted ? Material.accent : Material.foreground
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            indicator: Image {
                anchors.right: parent.right; anchors.rightMargin: parent.rightPadding
                anchors.verticalCenter: parent.verticalCenter
                height: parent.availableHeight
                width: parent.availableHeight
                source: !control.configMap[modelData].hasOwnProperty("id") ? "qrc:/icon-template" :
                    ((control.configMap[modelData]["is_power_on"] === "true" ||
                      control.configMap[modelData]["is_power_on"] === true) ? "qrc:/icon-power-on" : "qrc:/icon-power-off")
            }
            onClicked: {
                VMConfigSet.currentConfig = control.configMap[modelData]
                control.close()
            }
            ToolTip.visible: hovered
            ToolTip.text: control.configMap[modelData].hasOwnProperty("name") ? control.configMap[modelData]["name"] : ""
            ToolTip.delay: appTipDelay
            ToolTip.timeout: appTipTimeout
         }

        footerPositioning: ListView.OverlayFooter
        footer: ToolBar {
            z: 2; position: ToolBar.Footer
            Column {
                Repeater {
                    model: control.actionModel
                    ItemDelegate {
                        width: control.availableWidth
                        padding: appTextPadding
                        spacing: padding
                        action: modelData
                        indicator: Text {
                            visible: !isMobile
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            padding: parent.padding
                            font: parent.font
                            text: SystemHelper.shortcutText(modelData.shortcut)
                            color: Material.foreground
                        }
                        onClicked: control.close()
                    }
                }
            }
        }
        ScrollIndicator.vertical: ScrollIndicator {}
    }
}
