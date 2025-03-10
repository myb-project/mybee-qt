import QtQuick 2.15
import QtQuick.Controls 2.15

import CppCustomModules 1.0

TextRender {
    id: control

    property int cutAfter: height
    onCutAfterChanged: redraw()
    onDisplayBufferChanged: {
        cutAfter = height
        y = 0
    }
    contentItem: Item {
        anchors.fill: parent
        Behavior on opacity { NumberAnimation { easing.type: Easing.InOutQuad }}
        Behavior on y { NumberAnimation { easing.type: Easing.InOutQuad }}
    }
    cellDelegate: Rectangle {}
    cellContentsDelegate: Text {
        id: text
        property bool blinking: false
        textFormat: Text.PlainText
        opacity: blinking ? 0.5 : 1.0
        SequentialAnimation {
            running: text.blinking
            loops: Animation.Infinite
            NumberAnimation { target: text; property: "opacity"; to: 0.8; duration: 200 }
            PauseAnimation { duration: 400 }
            NumberAnimation { target: text; property: "opacity"; to: 0.5; duration: 200 }
        }
    }
    cursorDelegate: Rectangle {
        id: cursor
        property bool blinking: termFadeCursor
        NumberAnimation on opacity {
            running: cursor.blinking && Qt.application.state === Qt.ApplicationActive
            loops: Animation.Infinite; from: 1.0; to: 0.3; duration: 1200
            onStopped: Qt.callLater(function() { cursor.opacity = 1.0 })
        }
    }
    selectionDelegate: Rectangle { opacity: 0.5; color: termSelection }
    onVisualBell: if (termVisualBell) bellTimer.start()
    Keys.onShortcutOverride: function(event) { event.accepted = event.matches(StandardKey.Cancel) }

    Timer {
        id: bellTimer
        interval: 100
    }
    Rectangle {
        id: bellTimerRect
        anchors.fill: parent
        visible: opacity > 0
        opacity: bellTimer.running ? 0.5 : 0.0
        color: "white"
        Behavior on opacity { NumberAnimation {}}
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: appContextMenu.popup()
        onWheel: function(wheel) {
            if (wheel.angleDelta.y < 0) vbar.decrease()
            else vbar.increase()
        }
    }
    ScrollBar {
        id: vbar
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        orientation: Qt.Vertical
        hoverEnabled: true
        active: hovered || pressed || parent.contentHeight > parent.height
        size: parent.height / parent.contentHeight
        rotation: 180
    }
    contentY: Math.round(vbar.position * contentHeight)
}
