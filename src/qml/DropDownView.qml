import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import QmlCustomModules 1.0

MyFlickable {
    id: control

    required property string myClassName
    required property var model
    property alias header: headerLoader.sourceComponent

    function setIndexEnable(index, enable, opened) {
        var item = viewRepeater.itemAt(index)
        if (item) {
            item.opened = opened !== undefined ? opened : enable
            item.enabled = enable
            if (enable) {
                appDelay(appTinyDelay, function() {
                    if (!atYEnd && contentHeight > height)
                        contentY = contentHeight - height
                })
            }
        }
    }

    contentWidth: viewColumn.width
    contentHeight: viewColumn.height

    Column {
        id: viewColumn
        width: control.width
        spacing: 10

        Loader {
            id: headerLoader
        }

        Repeater {
            id: viewRepeater
            model: control.model

            Column {
                enabled: false
                property bool opened: false

                CheckDelegate {
                    id: listButton
                    width: viewColumn.width
                    checked: enabled && parent.opened
                    highlighted: checked
                    icon.source: modelData.icon ? modelData.icon : ""
                    text: qsTranslate(control.myClassName, modelData.text)
                    font.pointSize: appTitleSize
                    indicator: TintedImage {
                        x: parent.width - width - parent.rightPadding
                        y: parent.topPadding + (parent.availableHeight - height) / 2
                        source: "qrc:/icon-turn-right"
                        rotation: listPane.height / listPane.implicitHeight * 90
                        color: parent.checked ? Material.accent : Material.foreground
                    }
                    Rectangle {
                        anchors { left: parent.left; right: parent.right;  bottom: parent.bottom }
                        height: parent.checked ? 2 : 1
                        opacity: parent.checked ? 1.0 : 0.3
                        color: parent.checked ? Material.accent : Material.foreground
                    }
                }

                DropDownPane {
                    id: listPane
                    width: viewColumn.width
                    padding: modelData.hasOwnProperty("padding") ? modelData.padding : 5
                    highlight: true
                    show: listButton.checked
                    Column {
                        spacing: viewColumn.spacing
                        Repeater {
                            model: modelData.list
                            RowLayout {
                                width: listPane.availableWidth
                                spacing: 0
                                Label {
                                    Layout.fillWidth: text
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pointSize: modelData.item ? appWindow.font.pointSize : appTipSize
                                    text: modelData.text ? qsTranslate(control.myClassName, modelData.text) : ""
                                    elide: Text.ElideRight
                                }
                                Loader {
                                    Layout.fillWidth: !modelData.text
                                    Layout.minimumWidth: modelData.item ? listPane.availableWidth * 0.75 : 0
                                    active: listPane.show
                                    sourceComponent: modelData.item
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ScrollBar.vertical: ScrollBar { }
}
