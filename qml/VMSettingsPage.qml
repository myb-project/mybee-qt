import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("VM Settings")

    Component.onCompleted: {
        var map = VMConfigSet.configMap()
        for (var key in map) setupTabView(map[key])
        if (swipeView.count) {
            var index = 0, name = VMConfigSet.valueAt("alias")
            for (var i = 0; i < tabBar.count; i++) {
                if (tabBar.itemAt(i).text === name) {
                    index = i
                    break
                }
            }
            control.setCurrentIndex(index)
        } else Qt.callLater(setupProfile)
    }

    function setupTabView(obj) : bool {
        var cfg = Object.assign({}, obj)
        var item = Qt.createComponent("VMConfigPane.qml").createObject(swipeView, { "vmConfig": cfg })
        if (!item) {
            appToast(qsTr("Can't load VMConfigPane.qml"))
            return
        }
        tabBar.addItem(tabButtonComponent.createObject(tabBar, { text: cfg["alias"], tipText: cfg["name"] }))
        swipeView.addItem(item)
        control.setCurrentIndex(-1) // set to last index
    }

    property string setupServer
    function setupProfile() {
        var page = appPage("VMProfilesPage.qml")
        if (!page) return
        page.accepted.connect(function() {
            control.setupServer = page.currentServer
            var list = SystemHelper.loadArray(SystemHelper.fileName(control.setupServer) + "/profiles")
            var index = list.findIndex(obj => obj.profile === page.currentProfile["profile"])
            if (~index) {
                var obj = list[index]
                obj["server"] = control.setupServer
                obj["alias"] = VMConfigSet.uniqueValue("alias", obj["default_jname"] ? obj["default_jname"] : obj["type"])
                control.setupTabView(obj)
                appDelay(1000, function() {
                    if (!VMConfigSet.getList(control.setupServer))
                        appPage("AuthSettingsPage.qml", { currentServer: control.setupServer })
                })
            }
        })
    }

    function removeTabView(index) {
        var tab = tabBar.itemAt(index)
        var item = swipeView.itemAt(index)
        if (tab && item && VMConfigSet.removeConfig(item.vmConfig)) {
            tabBar.removeItem(tab)
            swipeView.removeItem(item)
            control.setCurrentIndex(-1) // set to last index
        }
    }

    function setCurrentIndex(index) {
        console.assert(swipeView.count === tabBar.count)
        if (index < 0 || index >= tabBar.count) index = tabBar.count - 1
        if (~index) {
            var button = tabBar.itemAt(index)
            if (button) button.toggle()
        }
    }

    Component {
        id: tabButtonComponent
        TabButton {
            required property string tipText
            rightPadding: spacing + removeTabButton.width
            width: implicitWidth
            ToolTip.visible: tipText && hovered
            ToolTip.text: tipText
            ToolTip.delay: appTipDelay
            ToolTip.timeout: appTipTimeout
            ToolButton {
                id: removeTabButton
                anchors.right: parent.right
                focusPolicy: Qt.NoFocus
                icon.source: "qrc:/icon-close"
                highlighted: tabBar.currentItem == parent
                onClicked: {
                    appWarning(qsTr("Do you want to remove VM %1?").arg(parent.text),
                               Dialog.Yes | Dialog.No).accepted.connect(function() {
                        control.removeTabView(parent.TabBar.index)
                    })
                }
            }
        }
    }

    header: Column {
        Pane {
            width: parent.width
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: swipeView.currentItem && !swipeView.currentItem.isCreated ? "qrc:/image-settings-ok"
                                                                                      : "qrc:/image-settings"
                    text: qsTr("Visit Wikipedia")
                    onClicked: Qt.openUrlExternally("https://wikipedia.org/wiki/Virtual_machine")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Configuring the virtual machine")
                }
            }
        }
        Row {
            TabBar {
                id: tabBar
                clip: true
                width: control.width - addNewButton.width
                currentIndex: swipeView.currentIndex
            }
            ToolButton {
                id: addNewButton
                focusPolicy: Qt.NoFocus
                enabled: tabBar.count <= 40
                icon.source: "qrc:/icon-new-tab"
                onClicked: control.setupProfile()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Create a new VM")
                ToolTip.delay: appTipDelay
                ToolTip.timeout: appTipTimeout
            }
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        focus: true // turn-on active focus here
        contentItem.focus: true // propagate to children items
    }
    Label {
        visible: !swipeView.count
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: appTitleSize
        wrapMode: Text.Wrap
        text: qsTr("Click the button above to create a new virtual machine")
    }

    footer: DropDownPane {
        show: swipeView.currentItem && (!swipeView.currentItem.isCreated || swipeView.currentItem.modified)
        enabled: show && swipeView.currentItem.isValid
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: enabled && swipeView.currentItem.isCreated ? qsTr("Save VM config?") : qsTr("Create a new VM?")
                elide: Text.ElideRight
            }
            DialogButtonBox {
                position: DialogButtonBox.Footer
                standardButtons: DialogButtonBox.Ok
                onAccepted: {
                    var config = swipeView.currentItem.vmConfig ? swipeView.currentItem.vmConfig : {}
                    if (!config["name"] || !config["alias"]) {
                        appInfo(qsTr("Please specify the name and alias of the VM"))
                        return
                    }
                    if (!swipeView.currentItem.isCreated && !VMConfigSet.isUniqueValue("alias", config["alias"])) {
                        appInfo(qsTr("The alias %1 already exist\n\nPlease choose another alias").arg(config["alias"]))
                        return
                    }
                    if (!VMConfigSet.isCbsdExecutable(config["server"]) && !RestApiSet.isSshKeyExist(config["server"])) {
                        appPage("AuthSettingsPage.qml", { currentServer: config["server"] })
                        return
                    }
                    if (!VMConfigSet.updateCurrent(config)) {
                        appError(qsTr("Unable to update the current configuration"))
                        return
                    }
                    if (!swipeView.currentItem.isCreated && !VMConfigSet.createVm(config)) {
                        appError(qsTr("No valid SSH key"))
                        return
                    }
                    appStackView.pop()
                }
            }
        }
    }
}
