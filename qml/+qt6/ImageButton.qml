import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

AbstractButton {
    id: control
    padding: 0
    display: AbstractButton.IconOnly
    scale: down ? 0.9 : (hovered ? 1.1 : 1.0)
    implicitHeight: appButtonSize
    implicitWidth: appButtonSize

    required property url source

    background: Image {
        id: backgroundImage
        source: control.source
        fillMode: Image.PreserveAspectFit
    }

    Desaturate {
        anchors.fill: backgroundImage
        source: backgroundImage
        desaturation: control.enabled ? 0.0 : 1.0
    }

    /*BrightnessContrast {
        anchors.fill: backgroundImage
        source: backgroundImage
        contrast: control.enabled ? 0.0 : -0.5
        brightness: control.enabled && (control.hovered || control.visualFocus) ? 0.3 : 0
    }*/

    ToolTip.visible: control.text && control.hovered
    ToolTip.text: control.text
    ToolTip.delay: appTipDelay
    ToolTip.timeout: appTipTimeout
}
