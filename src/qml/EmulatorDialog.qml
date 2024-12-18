import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

MyDialog {
    id: control
    title: qsTr("VM Emulator")
    standardButtons: Dialog.Cancel | Dialog.Ok
    padding: 0

    required property string currentServer
    property var capabilities: []

    property int currentIndex: -1
    property var currentEngine: (currentIndex >= 0 && currentIndex < engineListModel.count) ?
                                    engineListModel.get(currentIndex) : {}

    onAboutToShow: {
        currentIndex = -1
        engineListModel.clear()
        var list = capabilities.length ? capabilities : SystemHelper.loadArray(SystemHelper.fileName(currentServer) + "/capabilities")
        for (var i = 0; i < list.length; i++) {
            if (list[i].hasOwnProperty("name") && list[i].hasOwnProperty("prefix")) {
                engineListModel.append(list[i])
                if (currentIndex < 0 && list[i]["available"])
                    currentIndex = i
            }
        }
    }

    ListModel {
        id: engineListModel
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: viewColumn.width
        contentHeight: viewColumn.height

        Column {
            id: viewColumn
            spacing: 10

            Pane {
                width: control.availableWidth
                RowLayout {
                    anchors.fill: parent
                    ImageButton {
                        source: "qrc:/image-hypervisor"
                        text: qsTr("What is a hypervisor?")
                        onClicked: Qt.openUrlExternally("https://en.wikipedia.org/wiki/Hypervisor")
                    }
                    Label {
                        Layout.fillWidth: true
                        font.pointSize: appTitleSize
                        wrapMode: Text.Wrap
                        text: qsTr("Select the emulator to use for the new virtual machine at <i>%1</i>").arg(control.currentServer)
                    }
                }
            }

            GridLayout {
                x: (control.availableWidth - width) / 2
                enabled: ~control.currentIndex
                flow: GridLayout.TopToBottom
                rows: engineListModel.count
                columnSpacing: viewColumn.spacing * 2

                Repeater {
                    model: engineListModel
                    ImageButton {
                        Layout.maximumHeight: appIconSize
                        enabled: text
                        source: "qrc:/image-emulator-" + name
                        text: info
                        onClicked: Qt.openUrlExternally(text)
                    }
                }
                Repeater {
                    model: engineListModel
                    RadioButton {
                        enabled: available
                        checked: index === control.currentIndex
                        text: name.toUpperCase()
                        ToolTip.visible: ToolTip.text ? hovered : false
                        ToolTip.text: description
                        ToolTip.delay: appTipDelay
                        ToolTip.timeout: appTipTimeout
                        onToggled: if (checked) control.currentIndex = index
                    }
                }
            }
        }
    }
}
