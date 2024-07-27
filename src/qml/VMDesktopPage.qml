import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: desktopUrl.text
    enabled: desktopUrl.remote

    readonly property var qualityNames: [ "fast", "ave", "best" ]

    Component.onCompleted: {
        if (desktopUrl.remote) {
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
        location: VMConfigSet.desktopUrl()
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: !desktopView.frames ? 0 : 1
        ProgressPane {
            id: progressPane
            show: !desktopView.frames && !desktopView.error
            text: control.enabled ? qsTr("Connecting %1...").arg(desktopUrl.host) + "\n\n"
                                  : qsTr("No desktop available")
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
