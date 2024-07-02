import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: VMConfigSet.valueAt("ssh_string")
    enabled: VMConfigSet.isSshUser

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]

    // On Mac, Ctrl == Cmd. For Linux and Windows, Alt is more natural I think.
    //readonly property string tabChangeKey: Qt.platform.os === "osx" ? "Ctrl" : "Alt"

    property string sshUser: VMConfigSet.valueAt("ssh_user")
    property string sshHost: VMConfigSet.valueAt("ssh_host")
    property int sshPort:    parseInt(VMConfigSet.valueAt("ssh_port"))
    property string sshTerm: VMConfigSet.valueAt("ssh_term")
    property string sshKey:  ""
    property bool sshCheck:  false

    property string termPanLeft:    "\\e\\e[C"
    property string termPanRight:   "\\e\\e[D"
    property string termPanUp:      "\\e[6~"
    property string termPanDown:    "\\e[5~"
    property bool termFadeCursor:   true
    property bool termVisualBell:   true
    property string termBackground: "black"
    property string termSelection:  "lightGray"

    MySettings {
        id: termSettings
        category: control.myClassName
        property alias panLeft:     control.termPanLeft
        property alias panRight:    control.termPanRight
        property alias panUp:       control.termPanUp
        property alias panDown:     control.termPanDown
        property alias fadeCursor:  control.termFadeCursor
        property alias visualBell:  control.termVisualBell
        property alias background:  control.termBackground
        property alias selection:   control.termSelection
    }

    ListModel {
        id: sshListModel
        ListElement {
            text: qsTr("Term1")
        }
        onCountChanged: if (!count) appStackView.pop()
    }

    header: Row {
        TabBar {
            id: tabBar
            clip: true
            width: control.width - addNewButton.width
            currentIndex: swipeView.currentIndex
            Repeater {
                model: sshListModel
                TabButton {
                    focusPolicy: Qt.NoFocus
                    rightPadding: spacing + removeTabButton.width
                    width: implicitWidth
                    text: modelData
                    background: Rectangle { color: removeTabButton.highlighted ? control.termBackground : "transparent" }
                    ToolButton {
                        id: removeTabButton
                        anchors.right: parent.right
                        focusPolicy: Qt.NoFocus
                        icon.source: "qrc:/icon-close"
                        highlighted: tabBar.currentItem == parent
                        onClicked: sshListModel.remove(parent.TabBar.index)
                    }
                }
            }
        }
        ToolButton {
            id: addNewButton
            focusPolicy: Qt.NoFocus
            enabled: sshListModel.count < 5
            icon.source: "qrc:/icon-new-tab"
            property int lastNumber: sshListModel.count
            onClicked: {
                sshListModel.append({ text: qsTr("Term") + (++lastNumber).toString() })
                tabBar.currentIndex = sshListModel.count - 1
            }
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Open another terminal")
            ToolTip.delay: appTipDelay
            ToolTip.timeout: appTipTimeout
        }
    }

    Rectangle {
        anchors.fill: parent
        color: control.termBackground
        SwipeView {
            id: swipeView
            anchors.fill: parent
            currentIndex: tabBar.currentIndex
            focus: true // turn-on active focus here
            Repeater {
                model: sshListModel
                VMTerminalView {
                    onClosed: sshListModel.remove(index)
                }
            }
        }
    }

    /*Shortcut {
        sequence: "Ctrl+A"
        onActivated: print(sequence)
    }*/
}
