import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("VM Profiles")
    enabled: !VMConfigSet.isBusy

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    property string cloudServers

    // Json Object settings
    property var profileTypes: ({})
    property string currentServer
    property string currentType
    property var currentProfile: ({})
    property var lastStatus: ({})

    signal accepted()

    Component.onCompleted: {
        VMConfigSet.gotProfiles.connect(parseProfiles)

        cloudServers = SystemHelper.loadSettings(control.myClassName + "/cloudServers", "")
        if (!cloudServers) {
            var env = SystemHelper.envVariable("CLOUD_URL")
            if (Url.isRemoteAt(env)) cloudServers = Url.hostAt(env)
        }
        if (cloudServers) {
            var list = cloudServers.split(',').filter(Boolean)
            for (var item of list) {
                var host = item.trim()
                if (host) historyListModel.append({ "text": host })
            }
        } else {
            var folders = SystemHelper.fileList(null, null, true)
            for (var server of folders) {
                if (server !== VMConfigSet.cbsdName && RestApiSet.isSshKeyExist(server))
                    historyListModel.append({ "text": server })
            }
        }
        if (VMConfigSet.cbsdPath && historyListModel.find(VMConfigSet.cbsdPath) < 0)
            historyListModel.append({ "text": VMConfigSet.cbsdPath })

        dropDownView.setIndexEnable(0, true)
    }

    readonly property var dropDownModel: [
        {   // index 0
            icon: "",
            text: QT_TR_NOOP("Cloud server"),
            list: [ { text: QT_TR_NOOP("Location"), item: serverComponent } ]
        },{ // index 1
            icon: "",
            text: QT_TR_NOOP("Types"),
            padding: 0,
            list: [ { item: typesComponent } ]
        },{ // index 2
            icon: "",
            text: QT_TR_NOOP("Profiles"),
            padding: 0,
            list: [ { item: profilesComponent } ]
        },{ // index 3
            icon: "",
            text: QT_TR_NOOP("Properties"),
            padding: 0,
            list: [ { item: propertiesComponent } ]
        }
    ]

    function getProfiles(from) {
        historyListModel.prepend(from)
        VMConfigSet.getProfiles(from)
    }

    function parseProfiles(from, array) {
        control.currentType = ""
        control.currentProfile = {}
        var count = 0, types = {}
        for (var obj of array) {
            if (obj.hasOwnProperty("name") && obj.hasOwnProperty("type") && obj.hasOwnProperty("profile")) {
                var type = obj["type"]
                var list = types[type] ? types[type] : []
                list.push(obj)
                if (list.length > 1)
                    list.sort((a, b) => a.name.toLowerCase() > b.name.toLowerCase() ? 1 : -1)
                types[type] = list
                count++
            }
        }
        if (count) {
            control.currentServer = from
            control.profileTypes = types
            control.lastStatus = SystemHelper.loadObject(SystemHelper.fileName(from) + "/status")
            dropDownView.setIndexEnable(1, true)
        } else {
            control.currentServer = ""
            control.profileTypes = {}
            dropDownView.setIndexEnable(1, false)
            dropDownView.setIndexEnable(2, false)
            dropDownView.setIndexEnable(3, false)
            appInfo(qsTr("No compatible VM profiles"))
        }
    }


    Component {
        id: headerComponent
        Pane {
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-cloud"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://www.bsdstore.ru")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Create a virtual machine using the appropriate cloud server profile")
                }
            }
        }
    }


    // Cloud server

    ListModel {
        id: historyListModel
        function find(host) : int {
            var lower = host.toLowerCase()
            for (var i = 0; i < count; i++) {
                if (get(i).text.toLowerCase() === lower) return i
            }
            return -1
        }
        function prepend(host) {
            if (!host) return
            for (var i = 0; i < count; i++) {
                if (get(i).text !== host) continue
                if (i) {
                    move(i, 0, 1)
                    save()
                }
                return
            }
            insert(0, { text: host })
            if (count > 10) remove(10)
            save()
        }
        function save() {
            var line = ""
            for (var i = 0; i < count; i++) {
                if (line) line += ", "
                line += get(i).text
            }
            if (line !== control.cloudServers) control.cloudServers = line
        }
    }

    Component {
        id: serverComponent
        RowLayout {
            ComboBox {
                id: serverComboBox
                Layout.fillWidth: true
                selectTextByMouse: true
                editable: true
                //focus: !isMobile
                model: historyListModel
                onActivated: getProfiles(currentText)
                onAccepted: getProfiles(editText)
            }
            RoundButton {
                id: serverButton
                enabled: serverComboBox.editText && serverComboBox.editText !== control.currentServer
                highlighted: ~serverComboBox.editText.indexOf('.')
                icon.source: "qrc:/icon-enter"
                onClicked: getProfiles(serverComboBox.editText)
            }
            Component.onCompleted: {
                if (serverComboBox.currentText)
                    serverButton.forceActiveFocus()
                else contentItem.forceActiveFocus()
            }
        }
    }

    // OS types and profiles

    /*component HeaderTemplate: Text {
        width: control.availableWidth
        height: appRowHeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: control.font.family
        font.pointSize: appTipSize
        color: Material.foreground
        wrapMode: Text.Wrap
    }*/

    Component {
        id: itemDelegateComponent
        ItemDelegate {
            width: ListView.view.width
            height: appRowHeight
            highlighted: ListView.isCurrentItem
            text: modelData.name ? modelData.name : modelData
            contentItem: Text {
                font: parent.font
                text: parent.text
                color: parent.highlighted ? Material.accent : Material.foreground
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: ListView.view.currentIndex = index
        }
    }

    Component {
        id: typesComponent
        ListView {
            clip: true
            implicitWidth: control.availableWidth
            implicitHeight: Math.min(count, 10) * appRowHeight
            //header: HeaderTemplate { text: qsTr("Select the operating system type for the VM") }
            model: Object.keys(control.profileTypes).sort()
            delegate: itemDelegateComponent
            ScrollIndicator.vertical: ScrollIndicator {}
            onCurrentIndexChanged: {
                dropDownView.setIndexEnable(3, false)
                if (~currentIndex) {
                    control.currentType = model[currentIndex]
                    dropDownView.setIndexEnable(2, true)
                } else {
                    control.currentType = ""
                    control.currentProfile = {}
                    dropDownView.setIndexEnable(2, false)
                }
            }
            onModelChanged: {
                if (control.lastStatus.hasOwnProperty("vm_os_type")) {
                    for (var i = 0; i < count; i++) {
                        if (model[i] === control.lastStatus["vm_os_type"]) {
                            currentIndex = i
                            return
                        }
                    }
                }
                currentIndex = -1
            }
        }
    }

    Component {
        id: profilesComponent
        ListView {
            clip: true
            implicitWidth: control.availableWidth
            implicitHeight: Math.min(count, 10) * appRowHeight
            //header: HeaderTemplate { text: qsTr("Select the operating system profile for the VM") }
            model: control.profileTypes[control.currentType]
            delegate: itemDelegateComponent
            ScrollIndicator.vertical: ScrollIndicator {}
            onCurrentIndexChanged: {
                if (~currentIndex) {
                    control.currentProfile = model[currentIndex]
                    dropDownView.setIndexEnable(3, true, false)
                } else {
                    control.currentProfile = {}
                    dropDownView.setIndexEnable(3, false)
                }
            }
            onModelChanged: {
                if (control.lastStatus.hasOwnProperty("vm_os_profile")) {
                    for (var i = 0; i < count; i++) {
                        if (model[i]["profile"] === control.lastStatus["vm_os_profile"]) {
                            currentIndex = i
                            return
                        }
                    }
                }
                currentIndex = -1
            }
        }
    }

    // Properties

    TextMetrics {
        id: textMetrics
        font: control.font
        text: "WWWWWWWWWW"
    }
    Component {
        id: propertiesComponent
        ListView {
            clip: true
            implicitWidth: control.availableWidth
            implicitHeight: Math.min(count, 10) * (textMetrics.height + 4)
            model: Object.keys(control.currentProfile)
            currentIndex: -1
            delegate: Row {
                padding: 2
                height: textMetrics.height + 4 // double Row padding
                Text {
                    width: textMetrics.width
                    font: textMetrics.font
                    text: modelData
                    color: Material.accent
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: parent.ListView.view.width - textMetrics.width - 4 // double Row padding
                    font: control.font
                    text: control.currentProfile[modelData]
                    color: Material.foreground
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }

    DropDownView {
        id: dropDownView
        anchors.fill: parent
        myClassName: control.myClassName
        model: control.dropDownModel
        header: headerComponent
    }

    footer: DropDownPane {
        show: control.currentServer && control.currentProfile.hasOwnProperty("profile")
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Use this profile?")
                elide: Text.ElideRight
            }
            DialogButtonBox {
                position: DialogButtonBox.Footer
                standardButtons: DialogButtonBox.Ok
                onAccepted: {
                    SystemHelper.saveSettings(control.myClassName + "/cloudServers", control.cloudServers)
                    control.accepted()
                    appStackView.pop()
                }
            }
        }
    }
}
