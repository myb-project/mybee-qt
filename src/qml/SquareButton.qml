import QtQuick 2.15
import QtQuick.Controls 2.15

RoundButton {
    id: control

    property bool stretchable: false
    font: textMetric.font
    radius: appTextPadding
    display: AbstractButton.TextUnderIcon
    implicitWidth: stretchable ? Math.max(implicitHeight, textMetric.width + leftPadding + rightPadding)
                               : implicitHeight

    ToolTip.visible: ToolTip.text ? hovered : false
    ToolTip.delay: appTipDelay
    ToolTip.timeout: appTipTimeout

    TextMetrics {
        id: textMetric
        font.family: appWindow.font.family
        font.pointSize: appTipSize
        //font.bold: true
        text: control.text
    }
}
