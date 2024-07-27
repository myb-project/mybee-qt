import QtQuick 2.15
import QtQuick.Controls 2.15
//import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    enabled: VMConfigSet.isSshUser
    title: enabled ? VMConfigSet.valueAt("ssh_string") : qsTr("No Ssh settings")

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
        if (!control.enabled) {
            progressPane.text = control.title
            return
        }
        var key = VMConfigSet.valueAt("ssh_key")
        if (!SystemHelper.isSshPrivateKey(key)) {
            progressPane.text = qsTr("There is no private SSH key in %1").arg(key)
            return;
        }
        var ssh = SshProcess.sshSettings(VMConfigSet.valueAt("ssh_user"),
                                         VMConfigSet.valueAt("ssh_host"),
                                         parseInt(VMConfigSet.valueAt("ssh_port")),
                                         key)
        ssh.termType = VMConfigSet.valueAt("ssh_term")
        if (!ssh.isValid()) {
            progressPane.text = qsTr("Invalid SSH settings: %1").arg(ssh.toString)
            return;
        }
        sshSession.settings = ssh
        sshSession.connectToHost()
    }

    SshSession {
        id: sshSession
        //logLevel: SshSession.LogLevelFunctions -- use env var LIBSSH_DEBUG=[0..4] instead
        onStateChanged: {
            switch (state) {
            case SshSession.StateClosed:
                progressPane.text += qsTr("Connection closed")
                break
            case SshSession.StateLookup:
                progressPane.text += qsTr("Lookup for %1").arg(settings.host)
                break
            case SshSession.StateConnecting:
                progressPane.text += qsTr("Connecting %1").arg(url)
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
                progressPane.text += qsTr("Connection timed out (max %1 sec)").arg(settings.timeout);
                break
            case SshSession.StateClosing:
                progressPane.text += qsTr("Closing connection")
                break
            }
            progressPane.text += "\n\n"
        }
        onHostConnected: progressPane.text += qsTr("Connected to %1 (%2)").arg(hostAddress).arg(settings.port) + "\n\n"
        onHostDisconnected: appStackView.pop()
        onHelloBannerChanged: progressPane.text += helloBanner + "\n\n"
        onPubkeyHashChanged: progressPane.text += qsTr("Host public key hash:\n%1").arg(pubkeyHash) + "\n\n"
        onKnownHostChanged: sshServerDialog(sshSession)
        onAskQuestions: function(info, prompts) { sshPasswordDialog(sshSession, info, prompts) }
    }

    ListModel {
        id: sshListModel
        function addShellView() {
            append({ text: qsTr("Term %1").arg(sshSession.openChannels + 1) })
            tabBar.currentIndex = count - 1
            infoButton.checked = false
        }
    }

    header: RowLayout {
        spacing: 0
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            Repeater {
                model: sshListModel
                TabButton {
                    focusPolicy: Qt.NoFocus
                    rightPadding: spacing + removeTabButton.width
                    width: implicitWidth
                    height: tabBar.height
                    text: modelData
                    background: Rectangle {
                        color: (parent.checked && !infoButton.checked) ? control.termBackground : "transparent"
                    }
                    Component.onCompleted: if (tabBar.count === 1) checked = true
                    onClicked: infoButton.checked = false
                    MyToolButton {
                        id: removeTabButton
                        anchors.right: parent.right
                        focusPolicy: Qt.NoFocus
                        icon.source: "qrc:/icon-close"
                        highlighted: parent.checked
                        onClicked: sshListModel.remove(parent.TabBar.index)
                    }
                }
            }
        }
        MyToolButton {
            id: infoButton
            enabled: swipeView.count
            checkable: true
            text: qsTr("Log")
            icon.source: "qrc:/icon-ssh-key"
            ToolTip.text: qsTr("Connection log")
            background: Rectangle {
                color: parent.checked ? control.termBackground : "transparent"
            }
        }
        MyToolButton {
            enabled: sshSession.established
            text: qsTr("Add")
            icon.source: "qrc:/icon-create"
            ToolTip.text: qsTr("Open another terminal")
            onClicked: sshListModel.addShellView()
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: (infoButton.checked || !swipeView.count) ? 0 : 1
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
            background: Rectangle { color: control.termBackground }
            onCurrentIndexChanged: {
                if (tabBar.currentIndex !== currentIndex)
                    tabBar.setCurrentIndex(currentIndex)
            }
            Repeater {
                id: termRepeater
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
