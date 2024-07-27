import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

Pane {
    id: control
    Material.background: "black"

    property bool show: true
    property bool active: false
    property alias text: progressText.text

    AnimatedImage {
        anchors.centerIn: parent
        visible: control.enabled && control.show
        playing: visible && control.active
        source: "qrc:/image-connecting"
    }

    Text {
        id: progressText
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        horizontalAlignment: Text.AlignHCenter
        visible: text
        font.family: control.font.family
        font.pointSize: appExplainSize
        font.italic: true
        color: Material.accent
        wrapMode: Text.Wrap
    }
}
