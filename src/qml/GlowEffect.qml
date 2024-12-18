import QtQuick 2.15
import QtGraphicalEffects 1.15

RectangularGlow {
    id: control

    property bool show: control.enabled

    visible: opacity > 0
    glowRadius: 10
    cornerRadius: Math.min(width, height) / 2.0 + glowRadius
    color: "white"
    state: "hidden"
    states: [
        State {
            name: "shown"
            when: control.show
            PropertyChanges {
                target: control
                opacity: 1.0
            }
        },
        State {
            name: "hidden"
            when: !control.show
            PropertyChanges {
                target: control
                opacity: 0.0
            }
        }
    ]
    transitions: Transition {
        NumberAnimation {
            property: "opacity"
            duration: 500
            easing.type: Easing.InOutQuad
        }
    }
}
