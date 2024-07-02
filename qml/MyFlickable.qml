import QtQuick 2.15

Flickable {
    id: control
    clip: true

    boundsBehavior: Flickable.OvershootBounds
    flickableDirection: Flickable.HorizontalAndVerticalFlick

    transform: [
        Translate {
            x: control.width > contentWidth ? Math.floor(control.width / 2 - contentWidth / 2) : 0
        },
        Scale {
            origin.x: control.width / 2
            origin.y: control.verticalOvershoot < 0 ? control.height : 0
            yScale: 1.0 - Math.abs(control.verticalOvershoot / 10) / control.height
        },
        Rotation {
            axis { x: 0; y: 1; z: 0 }
            origin.x: control.width / 2
            origin.y: control.height / 2
            angle: -Math.min(10, Math.max(-10, control.horizontalOvershoot / 10))
        }
    ]

    Keys.onPressed: function(event) {
        switch (event.key) {
        case Qt.Key_Up:
        case Qt.Key_PageUp:
            if (atYBeginning) return
            flick(0, 1000)
            break
        case Qt.Key_Home:
            if (atYBeginning) return
            contentY = 0
            break
        case Qt.Key_Down:
        case Qt.Key_PageDown:
            if (atYEnd) return
            flick(0, -1000)
            break
        case Qt.Key_End:
            if (atYEnd) return
            contentY = Math.max(0, contentHeight - height)
            break
        case Qt.Key_Left:
            if (atXBeginning) return
            flick(1000, 0)
            break
        case Qt.Key_Right:
            if (atXEnd) return
            flick(-1000, 0)
            break
        default: return
        }
        event.accepted = true
    }
}
