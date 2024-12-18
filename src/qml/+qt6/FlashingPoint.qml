import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

Rectangle {
    id: control
    x: (parent instanceof ToolButton) ? parent.width / 2 - width / 2 + width : parent.width - width - appTextPadding
    y: parent.height - height - appTextPadding
    implicitWidth: diameter
    implicitHeight: diameter
    radius: diameter / 2
    color: "firebrick"

    property color foregroundColor: "white"
    property alias backgroundColor: control.color
    property bool flashing: visible && enabled
    readonly property int diameter: Math.max(textMetrics.tightBoundingRect.width,
                                             textMetrics.tightBoundingRect.height)

    TextMetrics {
        id: textMetrics
        font.family: appWindow.font.family
        font.pointSize: appExplainSize //appTitleSize
        font.bold: true
        text: "\u2713" // Unicode Character 'CHECK MARK'
    }

    RectangularGlow {
        anchors.fill: control
        color: control.foregroundColor
        glowRadius: control.diameter
        cornerRadius: Math.max(control.width, control.height) / 2 + glowRadius
    }

    Text {
        anchors.centerIn: parent
        color: control.foregroundColor
        text: textMetrics.text
    }

    ScaleAnimator on scale {
        running: control.flashing
        loops: Animation.Infinite
        from: 1.25; to: 0.75; duration: 1000
        onStopped: Qt.callLater(function() { control.scale = 1.0 })
    }
}
