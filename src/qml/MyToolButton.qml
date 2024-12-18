import QtQuick 2.15
import QtQuick.Controls 2.15

ToolButton {
    id: control

    spacing: 5
    focusPolicy: Qt.NoFocus
    display: appPortraitView ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
    font.pointSize: appTipSize
    font.bold: !appPortraitView

    ToolTip.visible: ToolTip.text ? hovered : false
    ToolTip.delay: appTipDelay
    ToolTip.timeout: appTipTimeout
}
