import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    enabled: desktopUrl.remote
    title: VMConfigSet.valueAt("name")

    readonly property var qualityNames: [ "fast", "ave", "best" ]
    readonly property list<Action> actionsList: [
        Action {
            checkable: true
            checked: !stackLayout.currentIndex
            text: qsTr("Progress log")
            onToggled: stackLayout.currentIndex = checked ? 0 : 1
        }
    ]

    Component.onCompleted: {
        desktopUrl.init()
        if (control.enabled) {
            var server = VMConfigSet.valueAt("server")
            if (Url.schemeAt(server) !== "ssh") {
                progressPane.text = qsTr("Connecting %1").arg(desktopUrl.text) + "\n\n"
                startDesktop()
            } else sshSession.connectToUrl(server, VMConfigSet.valueAt("ssh_key"))
        } else progressPane.text = qsTr("The desktop is unavailable")
    }

    function startDesktop() {
        if (!desktopUrl.password) {
            var dlg = appDialog("PasswordDialog.qml", { url: desktopUrl.location })
            if (dlg) {
                dlg.rejected.connect(appStackView.pop)
                dlg.accepted.connect(function() {
                    desktopUrl.password = dlg.password
                    desktopView.start(desktopUrl.location)
                })
            }
        } else desktopView.start(desktopUrl.location)
    }

    function configQuality() : int {
        var name = VMConfigSet.valueAt("quality").toLowerCase()
        if (name) {
            for (var i = 0; i < qualityNames.length; i++) {
                if (qualityNames[i] === name) return i
            }
        }
        return DesktopView.QualityAve
    }

    UrlModel {
        id: desktopUrl
        function init() {
            var desktop = VMConfigSet.valueAt("desktop").toLowerCase()
            if (VMConfigSet.isVncHost && (!desktop || desktop === "vnc")) {
                scheme = "vnc"
                userName = VMConfigSet.valueAt("vnc_user")
                if (!userName) userName = VMConfigSet.valueAt("ssh_user")
                password = VMConfigSet.valueAt("vnc_password")
                host = VMConfigSet.valueAt("vnc_host")
                port = parseInt(VMConfigSet.valueAt("vnc_port"))
            } else if (VMConfigSet.isRdpHost && (!desktop || desktop === "rdp")) {
                scheme = "rdp"
                userName = VMConfigSet.valueAt("rdp_user")
                if (!userName) userName = VMConfigSet.valueAt("ssh_user")
                password = VMConfigSet.valueAt("rdp_password")
                host = VMConfigSet.valueAt("rdp_host")
                port = parseInt(VMConfigSet.valueAt("rdp_port"))
            }
        }
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
                progressPane.text += qsTr("Tunneling %1").arg(desktopUrl.text)
                var pn = tunnel(desktopUrl.host, desktopUrl.port)
                if (pn) {
                    desktopUrl.host = "127.0.0.1"
                    desktopUrl.port = pn
                    startDesktop()
                } else progressPane.text += qsTr(" failed!")
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
        onHostDisconnected: {
            progressPane.text += qsTr("Host disconnected") + "\n\n"
            stackLayout.currentIndex = 0
        }
        onHelloBannerChanged: progressPane.text += helloBanner + "\n\n"
        onPubkeyHashChanged: progressPane.text += qsTr("Host public key hash:\n%1").arg(pubkeyHash) + "\n\n"
        onKnownHostChanged: sshServerDialog(sshSession)
        onAskQuestions: function(info, prompts) { sshPasswordDialog(sshSession, info, prompts) }
        //onOpenChannelsChanged: console.debug("openChannels", openChannels)
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: !desktopView.frames ? 0 : 1
        ProgressPane {
            id: progressPane
            active: sshSession.running
            show: !desktopView.frames && !desktopView.error
        }
        DesktopView {
            id: desktopView
            focus: true
            logging: false
            quality: configQuality()
            maxSize: appDesktopSize
            //bell: "service-bell.wav" // "short-pulse.wav" by default
            Keys.onShortcutOverride: function(event) {
                event.accepted = desktopView.alive ? event.matches(StandardKey.Cancel) : false
            }
            onDimensionChanged: {
                progressPane.text += qsTr("Desktop dimension %1x%2").arg(dimension.width).arg(dimension.height) + "\n\n"
                if (appCompactAction.checked) appCompactAction.toggle()
            }
            onAliveChanged: progressPane.text += qsTr("Wait for desktop loading...") + "\n\n"
            onErrorChanged: progressPane.text += error + "\n\n"
            onViewScaleChanged:  if (logging) console.debug("viewScale", viewScale)
            onViewCenterChanged: if (logging) console.debug("viewCenter", viewCenter)
            onViewRectChanged:   if (logging) console.debug("viewRect", viewRect)
        }
    }
}
