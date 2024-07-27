import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

import QmlCustomModules 1.0

Rectangle {
    id: control
    implicitWidth: (listView.orientation === ListView.Horizontal ? buttons : 1) * buttonSize
    implicitHeight: (listView.orientation === ListView.Vertical ? buttons : 1) * buttonSize
    color: MaterialSet.theme[Material.theme]["backlight"]

    property int buttons: listView.count
    property int buttonSize: appButtonSize + appTextPadding * 2
    readonly property int iconSize: buttonSize - appTextPadding * 3 - fontMetrics.height
    property alias orientation: listView.orientation
    property alias model: listView.model
    readonly property alias count: listView.count

    function ensureItemVisible(index) {
        listView.positionViewAtIndex(index, ListView.Center)
    }

    MouseArea { // preventing the propagation of mouse events
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
    }

    FontMetrics {
        id: fontMetrics
        font.family: appWindow.font.family
        font.pointSize: appTipSize
        font.bold: true
    }

    ListView {
        id: listView
        anchors.fill: parent
        orientation: ListView.Horizontal
        clip: true

        delegate: ItemDelegate {
            width: control.buttonSize
            height: control.buttonSize
            padding: appTextPadding
            spacing: padding
            opacity: enabled ? 1.0 : 0.3
            icon.color: "transparent"
            icon.height: control.iconSize
            icon.width: control.iconSize
            font: fontMetrics.font
            display: AbstractButton.TextUnderIcon
            action: modelData
        }
        ScrollIndicator.horizontal: ScrollIndicator {}
        ScrollIndicator.vertical: ScrollIndicator {}
    }

    Rectangle {
        x: listView.contentX > 0 ? listView.width - width : 0
        width: appTextPadding
        y: control.height / 2 - height / 2
        height: control.height * 0.66
        color: Material.accent
        visible: listView.orientation === ListView.Horizontal &&
                 listView.contentWidth > listView.width &&
                 (listView.atXBeginning || listView.atXEnd)
    }

    Rectangle {
        y: listView.contentY > 0 ? listView.height - height : 0
        height: appTextPadding
        x: control.width / 2 - width / 2
        width: control.width * 0.66
        color: Material.accent
        visible: listView.orientation === ListView.Vertical &&
                 listView.contentHeight > listView.height &&
                 (listView.atYBeginning || listView.atYEnd)
    }
}
