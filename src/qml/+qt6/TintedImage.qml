import QtQuick
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects

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
