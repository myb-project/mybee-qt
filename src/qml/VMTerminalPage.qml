import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: (swipeView.currentItem && swipeView.currentItem.title) ? swipeView.currentItem.title : VMConfigSet.valueAt("name")

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    readonly property string attachCmd: VMConfigSet.valueAt("attach_cmd")
    property var sshSession: null
    property font monoFont: control.font
    property list<Action> actionsList: [
        Action {
            id: progressLogAction
            enabled: control.sshSession
            checkable: true
            checked: swipeView.currentIndex === swipeView.count - 1
            text: qsTr("Progress log")
            onToggled: swipeView.setCurrentIndex(checked ? swipeView.count - 1 : 0)
        },
        Action {
            id: newTermAction
            enabled: (control.sshSession && control.sshSession.state === SshSession.StateReady) ||
                     (control.attachCmd && serverUrl.scheme === "file")
            text: qsTr("New Terminal")
            icon.source: "qrc:/icon-create"
            onTriggered: termListModel.addShellView()
        },
        Action {
            enabled: swipeView.currentIndex < swipeView.count - 1 && swipeView.currentItem.selected
            text: qsTr("Copy")
            icon.source: "qrc:/icon-content-select"
            shortcut: "Ctrl+Ins" // StandardKey.Copy -- should be transparent!
            onTriggered: if (swipeView.currentItem.copy()) appToast(qsTr("Copy to clipboard"))
        },
        Action {
            enabled: swipeView.currentIndex < swipeView.count - 1 && SystemHelper.clipboard
            text: qsTr("Paste")
            icon.source: "qrc:/icon-content-paste"
            shortcut: "Shift+Ins" // StandardKey.Paste  -- should be transparent!
            onTriggered: if (swipeView.currentItem.paste()) appToast(qsTr("Paste from clipboard"))
        }
    ]

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

    Component {
        id: dragModeComponent
        Action {
            enabled: swipeView.currentIndex < swipeView.count - 1
            checkable: true
            checked: !swipeView.interactive
            text: qsTr("Selection mode")
            onToggled: swipeView.interactive = !checked
        }
    }

    Component.onCompleted: {
        if (SystemHelper.isMobile)
            actionsList.push(dragModeComponent.createObject(control))

        monoFont.family = appFontFamily(appMonoFont ? appMonoFont : defaultMonoFont)
        monoFont.styleName = appFontStyle(appMonoFont ? appMonoFont : defaultMonoFont)
        monoFont.pointSize = appFontPointSize

        if (attachCmd) {
            if (serverUrl.scheme === "file")     termListModel.addShellView()
            else if (serverUrl.scheme === "ssh") sshSession = sshSessionComponent.createObject(control)
        } else if (VMConfigSet.isSshUser)        sshSession = sshSessionComponent.createObject(control)
    }

    UrlModel {
        id: serverUrl
        location: VMConfigSet.valueAt("server")
    }

    Component {
        id: sshSessionComponent
        SshSession {
            //logLevel: SshSession.LogLevelFunctions -- use env var LIBSSH_DEBUG=[0..4] instead
            settings {
                user:       control.attachCmd ? serverUrl.userName : VMConfigSet.valueAt("ssh_user")
                host:       control.attachCmd ? serverUrl.host : VMConfigSet.valueAt("ssh_host")
                port:       control.attachCmd ? serverUrl.port : parseInt(VMConfigSet.valueAt("ssh_port"))
                privateKey: VMConfigSet.valueAt("ssh_key")
                termType:   VMConfigSet.valueAt("ssh_term")
            }
            onRunningChanged: progressPane.active = running
            onStateChanged: {
                switch (state) {
                case SshSession.StateClosed:
                    progressPane.show = false
                    progressPane.text += qsTr("Connection closed")
                    break
                case SshSession.StateLookup:
                    progressPane.show = true
                    progressPane.text += qsTr("Lookup for %1").arg(settings.host)
                    break
                case SshSession.StateConnecting:
                    progressPane.show = true
                    progressPane.text += qsTr("Connecting %1").arg(sshUrl)
                    break
                case SshSession.StateServerCheck:
                    progressPane.show = true
                    progressPane.text += qsTr("Checking the server for safe use")
                    break
                case SshSession.StateUserAuth:
                    progressPane.show = true
                    progressPane.text += qsTr("User authentication")
                    break
                case SshSession.StateEstablished:
                    progressPane.show = true
                    if (!shareKey) return
                    progressPane.text += qsTr("Sending %1").arg(settings.privateKey + ".pub")
                    break
                case SshSession.StateReady:
                    progressPane.show = true
                    progressPane.text += qsTr("Requesting a remote shell")
                    appDelay(appTipDelay, termListModel.addShellView)
                    break
                case SshSession.StateError:
                    progressPane.show = false
                    progressPane.text += lastError
                    break
                case SshSession.StateDenied:
                    progressPane.show = false
                    progressPane.text += qsTr("Authentication failed")
                    break
                case SshSession.StateTimeout:
                    progressPane.show = false
                    progressPane.text += qsTr("Connection timed out (max %1 sec)").arg(settings.timeout)
                    break
                case SshSession.StateClosing:
                    progressPane.show = false
                    progressPane.text += qsTr("Closing connection")
                    break
                }
                progressPane.text += "\n\n"
            }
            onHostConnected: progressPane.text += qsTr("Connected to host %1 port %2").arg(hostAddress).arg(settings.port) + "\n\n"
            onHostDisconnected: {
                progressPane.text += qsTr("Host disconnected") + "\n\n"
                progressPane.show = true
            }
            onHelloBannerChanged: progressPane.text += helloBanner + "\n\n"
            onPubkeyHashChanged: progressPane.text += qsTr("Host public key hash:\n%1").arg(pubkeyHash) + "\n\n"
            onKnownHostChanged: sshServerDialog(control.sshSession)
            onAskQuestions: function(info, prompts) { sshPasswordDialog(control.sshSession, info, prompts) }
            //onOpenChannelsChanged: if (!openChannels) appDelay(appTipDelay, appStackView.pop)
            Component.onCompleted: connectToHost()
        }
    }

    ListModel {
        id: termListModel
        property int lastIndex: 0
        function addShellView() {
            append({ text: qsTr("Term %1").arg(++lastIndex) })
        }
        function delShellView(name) {
            for (var i = 0; i < count; i++) {
                if (get(i).text === name) {
                    remove(i)
                    break
                }
            }
        }
        onCountChanged: if (!count) appStackView.pop()
    }

    header: TabBar {
        currentIndex: swipeView.currentIndex
        Repeater {
            model: termListModel
            TabButton {
                focusPolicy: Qt.NoFocus
                rightPadding: spacing + removeTabButton.width
                width: implicitWidth
                down: swipeView.currentIndex === index
                text: modelData
                onClicked: swipeView.setCurrentIndex(index)
                MyToolButton {
                    id: removeTabButton
                    anchors.right: parent.right
                    focusPolicy: Qt.NoFocus
                    icon.source: "qrc:/icon-close"
                    highlighted: parent.down
                    onClicked: termListModel.delShellView(parent.text)
                }
            }
        }
        TabButton {
            width: implicitWidth
            focusPolicy: Qt.NoFocus
            display: AbstractButton.IconOnly
            action: newTermAction
            ToolTip.text: action.text
            ToolTip.visible: hovered
            ToolTip.delay: appTipDelay
            ToolTip.timeout: appTipTimeout
            onPressAndHold: {
                if (progressLogAction.enabled)
                    swipeView.setCurrentIndex(swipeView.count - 1)
            }
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        interactive: SystemHelper.isMobile
        focus: true // turn-on active focus here
        contentItem.focus: true // propagate active focus to SwipeView childrens
        background: Rectangle { color: control.termBackground }
        Repeater {
            model: termListModel
            VMTerminalView {
                session: control.sshSession
                font: control.monoFont
                dragMode: swipeView.interactive ? TextRender.DragScroll : TextRender.DragSelect
                onTerminalReady: {
                    progressPane.show = false
                    swipeView.setCurrentIndex(index)
                    if (control.attachCmd)
                        putString("exec " + control.attachCmd + '\r')
                }
                onHangupReceived: termListModel.remove(index)
                onKeyPressed: {
                    if (ctrlButtonKey.checked) ctrlButtonKey.toggle()
                    if (altButtonKey.checked) altButtonKey.toggle()
                }
            }
        }
        ProgressPane {
            id: progressPane
            enabled: control.sshSession
        }
    }

    component ButtonKeyTemplate: RoundButton {
        Layout.fillWidth: true
        focusPolicy: Qt.NoFocus
        radius: 5
        padding: 0
        spacing: 0
        font.pointSize: appTipSize
        font.bold: true
        highlighted: checked
    }

    footer: ToolBar {
        visible: SystemHelper.isMobile && Qt.inputMethod.visible
        RowLayout {
            anchors.fill: parent
            spacing: 0
            ButtonKeyTemplate {
                text: "Esc"
                onClicked: swipeView.currentItem.extKeyPressed(Qt.Key_Escape)
            }
            ButtonKeyTemplate {
                text: "Tab"
                onClicked: swipeView.currentItem.extKeyPressed(Qt.Key_Tab)
            }
            ButtonKeyTemplate {
                id: ctrlButtonKey
                checkable: true
                text: "Ctrl"
                onToggled: swipeView.currentItem.extKeyToggled(Qt.Key_Control, checked)
            }
            ButtonKeyTemplate {
                id: altButtonKey
                checkable: true
                text: "Alt"
                onToggled: swipeView.currentItem.extKeyToggled(Qt.Key_Alt, checked)
            }
            ButtonKeyTemplate {
                icon.source: "qrc:/icon-key-left"
                onClicked: swipeView.currentItem.extKeyPressed(Qt.Key_Left)
            }
            ButtonKeyTemplate {
                icon.source: "qrc:/icon-key-up"
                onClicked: swipeView.currentItem.extKeyPressed(Qt.Key_Up)
            }
            ButtonKeyTemplate {
                icon.source: "qrc:/icon-key-down"
                onClicked: swipeView.currentItem.extKeyPressed(Qt.Key_Down)
            }
            ButtonKeyTemplate {
                icon.source: "qrc:/icon-key-right"
                onClicked: swipeView.currentItem.extKeyPressed(Qt.Key_Right)
            }
        }
    }
}
