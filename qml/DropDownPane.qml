import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

Pane {
    id: control
    clip: true
    visible: height > 0
    padding: appTextPadding
    Material.background: MaterialSet.themeColor[Material.theme][highlight ? "highlight" :"backlight"]
    property bool highlight: false
    property bool show: true

    state: show ? "Shown" : "Hidden"
    states: [
        State {
            name: "Shown"
            PropertyChanges { target: control; height: control.implicitHeight }
            PropertyChanges { target: contentItem; y: control.topPadding }
        },
        State {
            name: "Hidden"
            PropertyChanges { target: control; height: 0 }
            PropertyChanges { target: contentItem; y: -control.implicitHeight }
        }
    ]
    transitions: Transition {
        SequentialAnimation {
            PropertyAction { property: "opacity"; value: 0.5 }
            ParallelAnimation {
                NumberAnimation { property: "height"; easing.type: Easing.InOutQuad }
                NumberAnimation { property: "y"; easing.type: Easing.InOutQuad }
            }
            PropertyAction { property: "opacity"; value: 1.0 }
        }
    }
}
