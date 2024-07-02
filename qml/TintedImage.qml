import QtQuick 2.15
import QtQuick.Controls.Material 2.15
import QtGraphicalEffects 1.15

Image {
    id: control

    fillMode: Image.PreserveAspectFit
    opacity: enabled ? 1.0 : 0.3

    property color color: Material.foreground

    layer.enabled: source.toString().startsWith("qrc:/")
    layer.effect: ColorOverlay {
        cached: true
        color: control.color
    }
}
