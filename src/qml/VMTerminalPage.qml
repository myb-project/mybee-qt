import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    enabled: VMConfigSet.isSshUser
    title: enabled ? sshSession.sshUrl : qsTr("No SSH settings")

    readonly property list<Action> actionsList: [
        Action {
            enabled: swipeView.count
            checkable: true
            checked: !stackLayout.currentIndex
            text: qsTr("Progress log")
            onToggled: stackLayout.currentIndex = checked ? 0 : 1
        },
        Action {
            enabled: sshSession.established
            text: qsTr("Open new")
            icon.source: "qrc:/icon-create"
            onTriggered: sshListModel.addShellView()
        }
    ]
    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]

    // On Mac, Ctrl == Cmd. For Linux and Windows, Alt is more natural I think.
    //readonly property string tabChangeKey: Qt.platform.os === "osx" ? "Ctrl" : "Alt"

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

    Component.onCompleted: {
        if (control.enabled) sshSession.connectToHost()
        else progressPane.text = control.title
    }

    SshSession {
        id: sshSession
        //logLevel: SshSession.LogLevelFunctions -- use env var LIBSSH_DEBUG=[0..4] instead
        settings {
            user:       VMConfigSet.valueAt("ssh_user")
            host:       VMConfigSet.valueAt("ssh_host")
            port:       parseInt(VMConfigSet.valueAt("ssh_port"))
            privateKey: VMConfigSet.valueAt("ssh_key")
            termType:   VMConfigSet.valueAt("ssh_term")
        }
        onStateChanged: {
            switch (state) {
            case SshSession.StateClosed:
                progressPane.text += qsTr("Connection closed")
                break
            case SshSession.StateLookup:
                progressPane.text += qsTr("Lookup for %1").arg(settings.host)
                break
            case SshSession.StateConnecting:
                progressPane.text += qsTr("Connecting %1").arg(sshUrl)
                break
            case SshSession.StateServerCheck:
                progressPane.text += qsTr("Checking the server for safe use")
                break
            case SshSession.StateUserAuth:
                progressPane.text += qsTr("User authentication")
                break
            case SshSession.StateEstablished:
                if (!shareKey) return
                progressPane.text += qsTr("Sending %1").arg(settings.privateKey + ".pub")
                break
            case SshSession.StateReady:
                progressPane.text += qsTr("Requesting a remote shell")
                appDelay(appTinyDelay, sshListModel.addShellView)
                break
            case SshSession.StateError:
                progressPane.text += lastError
                break
            case SshSession.StateDenied:
                progressPane.text += qsTr("Authentication failed")
                break
            case SshSession.StateTimeout:
                progressPane.text += qsTr("Connection timed out (max %1 sec)").arg(settings.timeout)
                break
            case SshSession.StateClosing:
                progressPane.text += qsTr("Closing connection")
                break
            }
            progressPane.text += "\n\n"
        }
        onHostConnected: progressPane.text += qsTr("Connected to %1 (%2)").arg(hostAddress).arg(settings.port) + "\n\n"
        onHostDisconnected: if (sshListModel.count) appStackView.pop()
        onHelloBannerChanged: progressPane.text += helloBanner + "\n\n"
        onPubkeyHashChanged: progressPane.text += qsTr("Host public key hash:\n%1").arg(pubkeyHash) + "\n\n"
        onKnownHostChanged: sshServerDialog(sshSession)
        onAskQuestions: function(info, prompts) { sshPasswordDialog(sshSession, info, prompts) }
        //onOpenChannelsChanged: console.debug("openChannels", openChannels)
    }

    ListModel {
        id: sshListModel
        function addShellView() {
            append({ text: qsTr("Term %1").arg(sshSession.openChannels + 1) })
            tabBar.currentIndex = count - 1
        }
    }

    header: TabBar {
        id: tabBar
        Repeater {
            model: sshListModel
            TabButton {
                focusPolicy: Qt.NoFocus
                rightPadding: spacing + removeTabButton.width
                width: implicitWidth
                height: tabBar.height
                down: checked
                text: modelData
                onClicked: stackLayout.currentIndex = 1
                Component.onCompleted: if (tabBar.count === 1) checked = true
                MyToolButton {
                    id: removeTabButton
                    anchors.right: parent.right
                    focusPolicy: Qt.NoFocus
                    icon.source: "qrc:/icon-close"
                    highlighted: parent.checked
                    onClicked: swipeView.itemAt(parent.TabBar.index).closeShell()
                }
                TapHandler {
                   acceptedButtons: Qt.RightButton
                   onTapped: {
                       appContextMenu.popup()
                   }
                }
            }
        }
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: !swipeView.count ? 0 : 1
        ProgressPane {
            id: progressPane
            active: sshSession.running
            show: sshSession.state === SshSession.StateConnecting ||
                  sshSession.state === SshSession.StateServerCheck ||
                  sshSession.state === SshSession.StateUserAuth
        }
        SwipeView {
            id: swipeView
            currentIndex: tabBar.currentIndex
            focus: true // turn-on active focus here
            contentItem.focus: true // propagate active focus to SwipeView childrens
            background: Rectangle { color: control.termBackground }
            onCurrentIndexChanged: {
                if (tabBar.currentIndex !== currentIndex)
                    tabBar.setCurrentIndex(currentIndex)
            }
            Repeater {
                model: sshListModel
                VMTerminalView {
                    session: sshSession
                    onShellOpened: forceActiveFocus()
                    onShellClosed: sshListModel.remove(index)
                }
            }
        }
    }
}
