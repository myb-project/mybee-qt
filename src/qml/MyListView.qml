import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

ListView {
    id: control
    clip: true

    readonly property int xVelocity: 1000 // horizontal pixels in seconds
    readonly property int yVelocity: 1000 // vertical pixels in seconds
    readonly property int pageVelocity: 2000 // vertical pixels in seconds
    readonly property int pageRows: height / (currentItem ? currentItem.height : 40)
    property bool underline: false

    function ensureCurrentVisible() {
        if (count && ~currentIndex)
            positionViewAtIndex(currentIndex, ListView.Contain)
    }

    boundsMovement: Flickable.StopAtBounds
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    snapMode: ListView.SnapToItem

    transform: [
        //Translate { x: control.width > contentWidth ? Math.floor(control.width / 2 - contentWidth / 2) : 0 },
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

    highlightMoveDuration: 250
    highlightResizeDuration: 0
    highlightFollowsCurrentItem: true
    highlightRangeMode: ListView.ApplyRange
    preferredHighlightBegin: 48
    preferredHighlightEnd: width - 48
    highlight: Item {
        visible: control.count && ~control.currentIndex
        z: 2
        Rectangle {
            height: 2
            width: parent.width
            y: parent.height - height
            color: Material.accent
        }
    }

    Keys.priority: Keys.AfterItem
    Keys.onPressed: function(event) {
        if (!control.count) {
            event.accepted = false
            return
        }
        switch (event.key) {
        case Qt.Key_Up:
            if ((event.modifiers & Qt.ControlModifier) && contentY > 0) contentY = 0
            else flick(0, (event.modifiers & Qt.ShiftModifier) ? control.yVelocity * 2 : control.yVelocity)
            break
        case Qt.Key_Down:
            if ((event.modifiers & Qt.ControlModifier) && contentHeight > height) contentY = contentHeight - height
            else flick(0, (event.modifiers & Qt.ShiftModifier) ? -control.yVelocity * 2 : -control.yVelocity)
            break
        case Qt.Key_Left:
            if ((event.modifiers & Qt.ControlModifier) && contentX > 0) contentX = 0
            else flick((event.modifiers & Qt.ShiftModifier) ? control.xVelocity * 2 : control.xVelocity, 0)
            break
        case Qt.Key_Right:
            if ((event.modifiers & Qt.ControlModifier) && contentWidth > width) contentX = contentWidth - width
            else flick((event.modifiers & Qt.ShiftModifier) ? -control.xVelocity * 2 : -control.xVelocity, 0)
            break
        case Qt.Key_PageUp:
            currentIndex = Math.max(currentIndex - pageRows, 0)
            positionViewAtIndex(currentIndex, ListView.Beginning)
            //flick(0, (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) ? control.pageVelocity * 2 : control.pageVelocity)
            break
        case Qt.Key_PageDown:
            currentIndex = Math.min(currentIndex + pageRows, count - 1)
            positionViewAtIndex(currentIndex, ListView.End)
            //flick(0, (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) ? -control.pageVelocity * 2 : -control.pageVelocity)
            break
        case Qt.Key_Home:
            currentIndex = 0
            positionViewAtBeginning()
            break
        case Qt.Key_End:
            currentIndex = count - 1
            positionViewAtEnd()
            break
        default:
            event.accepted = false
            return
        }
        event.accepted = true
    }
}
