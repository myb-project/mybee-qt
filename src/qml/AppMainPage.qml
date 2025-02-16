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
    property string collapseTree: ""
    property var collapseSet: new Set(collapseTree.split(' '))
    property var schemeIconMap: {
        "file":  "qrc:/icon-open-folder",
        "ssh":   "qrc:/icon-ssh-key",
        "http":  "qrc:/icon-cloud-server",
        "https": "qrc:/icon-cloud-server"
    }
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
                    } else if (VMConfigSet.isSshUser || VMConfigSet.isAttachCmd) {
                        appPage("VMTerminalPage.qml")
                    }
                }
                return
            }
        }
        console.error("itemActivated(%1): Bad Object").arg(index)
    }

    function setFilter(keys = []) {
        configListModel.filter = keys
        configListModel.update()
        if (!configListModel.filter.length)
            control.filterText = ""
    }

    ListModel {
        id: configListModel
        property var map: VMConfigSet.configMap
        property var filter: []
        onMapChanged: Qt.callLater(update)
        function update() {
            var list = Object.keys(map).sort(function(a, b) {
                var p1 = a.indexOf('@')
                var s1 = a.slice(p1 + 1)
                var p2 = b.indexOf('@')
                var s2 = b.slice(p2 + 1)
                if (s1 > s2) return 1
                if (s1 < s2) return -1
                return a.slice(0, p1) > b.slice(0, p2) ? 1 : -1
            })
            clear()
            for (var key of list) {
                if (filter.length && filter.indexOf(key) < 0) continue
                var obj = map[key]
                var server = obj["server"]
                if (!server) continue
                var icon = !obj.hasOwnProperty("id") ? "qrc:/icon-template" :
                    (obj["is_power_on"] === true || obj["is_power_on"] === "true") ? "qrc:/icon-power-on" : "qrc:/icon-power-off"
                configListModel.append({
                    "key": key,
                    "icon": icon,
                    "text": obj["alias"],
                    "tip": qsTr("%1 on %2 %3").arg(obj["name"]).arg(server).arg(obj["emulator"]),
                    "server": server,
                    "visible": !control.collapseSet.has(server)
                })
            }
            for (var i = 0; i < count; i++) {
                if (get(i).key === VMConfigSet.lastSelected) {
                    listView.currentIndex = i
                    listView.ensureCurrentVisible()
                    return
                }
            }
            listView.currentIndex = -1
        }
        function setVisible(server, visible) {
            for (var i = 0; i < count; i++) {
                if (get(i).server === server)
                    setProperty(i, "visible", visible)
            }
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
                enabled: control.filterText
                text: qsTr("Clear")
                icon.source: "qrc:/icon-filter-clear"
                ToolTip.text: qsTr("Clear filter")
                onClicked: setFilter()
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
                visible: model.visible
                padding: appTextPadding
                spacing: appTextPadding
                width: appPortraitView ? Math.max(control.buttonWidth, Math.floor(listView.width / Math.max(listView.count, 1))) : listView.width
                height: visible ? implicitHeight : 0
                display: appPortraitView ? AbstractButton.TextUnderIcon : AbstractButton.TextBesideIcon
                highlighted: ListView.isCurrentItem
                text: model.text
                icon.source: model.icon
                icon.color: "transparent"
                ToolTip.visible: hovered
                ToolTip.text: model.tip
                ToolTip.delay: appTipDelay
                ToolTip.timeout: appTipTimeout
                onClicked: {
                    if (ListView.isCurrentItem) return
                    listView.currentIndex = index
                    VMConfigSet.setCurrent(model.key)
                    VMConfigSet.lastSelected = model.key
                }
                onPressAndHold: if (configAction.enabled) appPage("VMConfigPage.qml")
                onDoubleClicked: control.itemActivated(index)
                Keys.onReturnPressed: control.itemActivated(index)
            }
            section {
                property: appPortraitView ? null : "server"
                delegate: ToolButton {
                    anchors.left: parent.left
                    focusPolicy: Qt.NoFocus
                    width: listView.width
                    checkable: true
                    checked: !control.collapseSet.has(text)
                    icon.source: schemeIconMap[Url.schemeAt(text)]
                    text: section
                    font.pointSize: appTitleSize
                    onToggled: {
                        if (checked) control.collapseSet.delete(text)
                        else control.collapseSet.add(text)
                        control.collapseTree = Array.from(control.collapseSet).join(' ').trim()
                        configListModel.setVisible(text, checked)
                    }
                }
            }
        }

        VMActivityView {
            id: vmActivityView
            SplitView.preferredWidth: Math.round(appPortraitView ? control.availableWidth : control.availableWidth * (1 - control.splitHFactor))
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !SystemHelper.isMobile
        acceptedButtons: Qt.RightButton
        onClicked: appContextMenu.popup()
    }
}
