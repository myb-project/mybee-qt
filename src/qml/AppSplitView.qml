import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

import CppCustomModules 1.0

SplitView {
    id: control
    orientation: appPortraitView ? Qt.Vertical : Qt.Horizontal

    property int buttonWidth: 100
    property int splitThickness: 3

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

    ListModel {
        id: configListModel
        property var map: VMConfigSet.configMap
        onMapChanged: Qt.callLater(update)
        function update() {
            clear()
            for (var key in map) {
                var obj = map[key]
                var icon = !obj.hasOwnProperty("id") ? "qrc:/icon-template" :
                    (obj["is_power_on"] === true || obj["is_power_on"] === "true") ? "qrc:/icon-power-on" : "qrc:/icon-power-off"
                configListModel.append({ "key": key, "icon": icon, "text": obj["alias"], "tip": obj["name"], "server": obj["server"] })
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

    MyListView {
        id: listView
        focus: true
        readonly property int buttonHeight: currentItem ? Math.round(currentItem.implicitHeight) : 0
        readonly property int viewHeight: Math.round(appPortraitView ? buttonHeight : control.availableHeight)
        SplitView.preferredWidth: count ? Math.round(appPortraitView ? control.availableWidth : control.availableWidth * 0.25) : 0
        SplitView.preferredHeight: viewHeight
        SplitView.maximumHeight: viewHeight
        orientation: appPortraitView ? ListView.Horizontal : ListView.Vertical
        contentWidth: (appPortraitView ? count : 1) * buttonWidth
        contentHeight: (appPortraitView ? 1 : count) * buttonHeight

        model: configListModel
        delegate: ItemDelegate {
            padding: appTextPadding
            spacing: appTextPadding
            width: appPortraitView ? Math.max(buttonWidth, Math.floor(ListView.view.width / Math.max(ListView.view.count, 1))) : ListView.view.width
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
            onPressAndHold: if (appConfigAction.enabled) appPage("VMConfigPage.qml")
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
                width: Math.min(ListView.view.width, implicitWidth)
                padding: appRowHeight / 2
                font.pointSize: appTitleSize
                text: section
                elide: Text.ElideRight
            }
        }
    }

    VMActivityView {
        id: vmActivityView
        SplitView.preferredWidth: Math.round(appPortraitView ? control.availableWidth : control.availableWidth * 0.75)
    }
}
