import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: desktopUrl.text
    enabled: desktopUrl.remote && desktopUrl.password

    readonly property var qualityNames: [ "fast", "ave", "best" ]

    Component.onCompleted: {
        if (desktopUrl.remote) {
            if (!desktopUrl.password) {
                var dlg = appDialog("UrlPasswordDialog.qml", { "location": desktopUrl.location })
                if (dlg) {
                    dlg.rejected.connect(function() { appStackView.pop() })
                    dlg.apply.connect(function(location) {
                        desktopUrl.location = location
                        desktopView.start(desktopUrl.location)
                    })
                }
            } else desktopView.start(desktopUrl.location)
        }
    }

    function configQuality() : int {
        if (VMConfigSet.currentConfig.hasOwnProperty("quality")) {
            var name = VMConfigSet.currentConfig["quality"].toLowerCase()
            for (var i = 0; i < qualityNames.length; i++) {
                if (qualityNames[i] === name) return i
            }
        }
        return DesktopView.QualityAve
    }

    UrlModel {
        id: desktopUrl
        location: VMConfigSet.desktopUrl()
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: !desktopView.frames ? 0 : 1
        Rectangle {
            color: "black"
            AnimatedImage {
                anchors.centerIn: parent
                visible: control.enabled
                source: "qrc:/image-connecting"
                playing: visible && !desktopView.frames && !desktopView.error
            }
            Text {
                id: connectingText
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                horizontalAlignment: Text.AlignHCenter
                font.family: control.font.family
                font.pointSize: appExplainSize
                font.italic: true
                color: Material.accent
                wrapMode: Text.Wrap
                text: control.enabled ? qsTr("Connecting %1...").arg(desktopUrl.host) + "\n\n" : qsTr("No desktop available")
            }
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
                connectingText.text += qsTr("Desktop dimension %1x%2").arg(dimension.width).arg(dimension.height) + "\n\n"
            }
            onAliveChanged: connectingText.text += qsTr("Wait for desktop loading...") + "\n\n"
            onErrorChanged: connectingText.text += error + "\n\n"

            onViewScaleChanged:  if (logging) console.debug("viewScale", viewScale)
            onViewCenterChanged: if (logging) console.debug("viewCenter", viewCenter)
            onViewRectChanged:   if (logging) console.debug("viewRect", viewRect)
        }
    }
}
