import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Page {
    id: control

    property int buttonWidth: 100
    property int splitThickness: 3
    property real splitHFactor: 0.25
    property int splitVFactor: 1
    property string filterText
    readonly property list<Action> actionsList: [
        Action {
            id: setFilterAction
            enabled: VMConfigSet.configCount
            text: qsTr("Set filter")
            icon.source: "qrc:/icon-filter-plus"
            onTriggered: {
                appPage("AppFilterPage.qml")
            }
        },
        Action {
            id: clearFilterAction
            enabled: control.filterText
            text: qsTr("Clear filter")
            icon.source: "qrc:/icon-filter-clear"
            onTriggered: {
                configListModel.filter.length = 0
                configListModel.update()
                control.filterText = ""
            }
        },
        Action {
            id: configAction
            enabled: VMConfigSet.isValid && !VMConfigSet.isPowerOn
            icon.source: "qrc:/icon-config"
            text: qsTr("Config VM")
            onTriggered: {
                appPage("VMConfigPage.qml")
            }
        },
        Action {
            enabled: VMConfigSet.isValid
            icon.source: "qrc:/icon-remove"
            text: qsTr("Remove VM")
            onTriggered: {
                appWarning(qsTr("Do you want to remove %1?").arg(VMConfigSet.valueAt("alias")),
                           Dialog.Yes | Dialog.No).accepted.connect(VMConfigSet.removeVm)
            }
        }
    ]

    function appendText(text) {
        vmActivityView.appendText(text)
    }

    function itemActivated(index) {
        var key = configListModel.get(index).key
        if (key) {
            var cfg = VMConfigSet.configMap[key]
            if (cfg && cfg["alias"]) {
                if (VMConfigSet.isCreated) {
                    if (!VMConfigSet.isPowerOn) {
                        appInfo(qsTr("Do you want to start %1?").arg(cfg["alias"]), Dialog.Yes | Dialog.No)
                                .accepted.connect(function() { VMConfigSet.startVm(cfg) })
                    } else appPage("VMTerminalPage.qml")
                }
                return
            }
        }
        console.error("itemActivated(%1): Bad Object").arg(index)
    }

    function setFilter(keys) {
        configListModel.filter = keys
        configListModel.update()
    }

    ListModel {
        id: configListModel
        property var map: VMConfigSet.configMap
        property var filter: []
        onMapChanged: Qt.callLater(update)
        function update() {
            clear()
            for (var key in map) {
                if (filter.length && filter.indexOf(key) < 0) continue
                var obj = map[key]
                var icon = !obj.hasOwnProperty("id") ? "qrc:/icon-template" :
                    (obj["is_power_on"] === true || obj["is_power_on"] === "true") ? "qrc:/icon-power-on" : "qrc:/icon-power-off"
                configListModel.append({
                    "key": key,
                    "icon": icon,
                    "text": obj["alias"],
                    "tip": qsTr("%1 on %2 %3").arg(obj["name"]).arg(obj["server"]).arg(obj["emulator"]),
                    "server": obj["server"]
                })
            }
            for (var i = 0; i < count; i++) {
                if (get(i).key === VMConfigSet.lastSelected) {
                    listView.currentIndex = i
                    return
                }
            }
            listView.currentIndex = -1
        }
    }

    header: ToolBar {
        visible: control.filterText
        RowLayout {
            anchors.fill: parent
            ItemDelegate {
                Layout.fillHeight: true
                spacing: 5
                action: setFilterAction
                text: control.filterText
                ToolTip.text: action.text
                ToolTip.visible: hovered
                ToolTip.delay: appTipDelay
                ToolTip.timeout: appTipTimeout
            }
            Item { Layout.fillWidth: true }
            MyToolButton {
                action: clearFilterAction
                text: qsTr("Clear")
                ToolTip.text: action.text
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: appPortraitView ? Qt.Vertical : Qt.Horizontal
        handle: Rectangle {
            implicitWidth: appPortraitView ? parent.height : control.splitThickness
            implicitHeight: appPortraitView ? control.splitThickness : parent.width
            color: Material.foreground
            opacity: SplitHandle.hovered || SplitHandle.pressed ? 1 : 0.3
            SplitHandle.onHoveredChanged: {
                if (!SplitHandle.hovered) SystemHelper.setCursorShape(-1)
                else SystemHelper.setCursorShape(appPortraitView ? Qt.SplitVCursor : Qt.SplitHCursor)
            }
        }
        onResizingChanged: {
            if (resizing) return
            if (!appPortraitView) {
                var hf = (listView.width / control.availableWidth).toFixed(2)
                if (hf < 0.1) hf = 0.1
                else if (hf > 0.9) hf = 0.9
                control.splitHFactor = hf
            } else control.splitVFactor = listView.height > listView.buttonHeight / 2 ? 1 : 0
        }

        MyListView {
            id: listView
            focus: true
            underline: true
            readonly property int buttonHeight: currentItem ? Math.round(currentItem.implicitHeight) : 0
            readonly property int viewHeight: Math.round(appPortraitView ? buttonHeight : control.availableHeight)
            SplitView.preferredWidth: count ? Math.round(appPortraitView ? control.availableWidth : control.availableWidth * control.splitHFactor) : 0
            SplitView.preferredHeight: (!appPortraitView || control.splitVFactor) ? viewHeight : 0
            SplitView.maximumHeight: viewHeight
            orientation: appPortraitView ? ListView.Horizontal : ListView.Vertical
            contentWidth: (appPortraitView ? count : 1) * control.buttonWidth
            contentHeight: (appPortraitView ? 1 : count) * buttonHeight

            model: configListModel
            delegate: ItemDelegate {
                padding: appTextPadding
                spacing: appTextPadding
                width: appPortraitView ? Math.max(control.buttonWidth, Math.floor(listView.width / Math.max(listView.count, 1))) : listView.width
                display: appPortraitView ? AbstractButton.TextUnderIcon : AbstractButton.TextBesideIcon
                highlighted: ListView.isCurrentItem
                text: model.text
                icon.source: model.icon
                icon.color: "transparent"
                ToolTip.visible: hovered
                ToolTip.text: model.tip
                ToolTip.delay: appTipDelay
                ToolTip.timeout: appTipTimeout
                onClicked: listView.currentIndex = index
                onPressAndHold: if (configAction.enabled) appPage("VMConfigPage.qml")
                onDoubleClicked: control.itemActivated(index)
                Keys.onReturnPressed: control.itemActivated(index)
            }
            onCurrentIndexChanged: {
                if (currentIndex >= 0 && currentIndex < configListModel.count &&
                        configListModel.get(currentIndex).key !== VMConfigSet.lastSelected) {
                    VMConfigSet.setCurrent(configListModel.get(currentIndex).key)
                    ensureCurrentVisible()
                }
            }
            section {
                property: appPortraitView ? null : "server"
                delegate: Label {
                    anchors.left: parent.left
                    width: Math.min(listView.width, implicitWidth)
                    padding: appRowHeight / 2
                    font.pointSize: appTitleSize
                    text: section
                    elide: Text.ElideRight
                }
            }
        }

        VMActivityView {
            id: vmActivityView
            SplitView.preferredWidth: Math.round(appPortraitView ? control.availableWidth : control.availableWidth * (1 - control.splitHFactor))
        }
    }

    TapHandler {
       acceptedButtons: Qt.RightButton
       onTapped: {
           appContextMenu.popup()
       }
    }
}
