import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("VM profile")
    enabled: !VMConfigSet.isBusy

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    required property string currentFolder
    property var lastServer: ({})
    property var profileTypes: ({})
    property string currentType
    property var currentProfile: ({})

    readonly property var dropDownModel: [
        {   // index 0
            icon: "",
            text: QT_TR_NOOP("Types"),
            padding: 0,
            list: [ { item: typesComponent } ]
        },{ // index 1
            icon: "",
            text: QT_TR_NOOP("Profiles"),
            padding: 0,
            list: [ { item: profilesComponent } ]
        },{ // index 2
            icon: "",
            text: QT_TR_NOOP("Properties"),
            padding: 0,
            list: [ { item: propertiesComponent } ]
        }
    ]

    Component.onCompleted: {
        lastServer = SystemHelper.loadObject(currentFolder + "/lastServer")
        if (!lastServer.hasOwnProperty("server") || !lastServer.hasOwnProperty("ssh_key")) {
            appError(qsTr("Can't load current configuration"))
            return
        }
        HttpRequest.recvError.connect(removeLastServer)
        SshProcess.execError.connect(removeLastServer)
        SystemProcess.execError.connect(removeLastServer)

        appDelay(appTinyDelay, VMConfigSet.getProfiles, lastServer)
    }

    Component.onDestruction: {
        HttpRequest.recvError.disconnect(removeLastServer)
        SshProcess.execError.disconnect(removeLastServer)
        SystemProcess.execError.disconnect(removeLastServer)
    }

    function removeLastServer() {
        if (currentFolder) SystemHelper.removeFile(currentFolder + "/lastServer.json")
    }

    Connections {
        target: VMConfigSet
        function onParseProfiles(url, array) {
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
            if (!count) {
                removeLastServer()
                appInfo(qsTr("No compatible VM profiles"))
                return
            }
            if (url !== lastServer["server"]) {
                console.warn("onParseProfiles: Unexpected URL change", lastServer["server"], "->", url)
                lastServer["server"] = url
            }
            currentType = ""
            profileTypes = types
            currentProfile = {}
            dropDownView.setIndexEnable(0, true)
        }
    }

    Component {
        id: headerComponent
        Pane {
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-profiles"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://www.bsdstore.ru")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Selecting the OS profile for the virtual machine being created at the <i>%1</i>").arg(lastServer["server"])
                }
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
            onClicked: {
                ListView.view.currentIndex = index
                if (modelData.profile)
                     SystemHelper.saveSettings(control.myClassName + "/lastProfile", modelData.profile)
                else SystemHelper.saveSettings(control.myClassName + "/lastType", modelData)
            }
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
            ScrollIndicator.vertical: ScrollIndicator { active: true }
            onCurrentIndexChanged: {
                dropDownView.setIndexEnable(2, false)
                if (~currentIndex) {
                    control.currentType = model[currentIndex]
                    dropDownView.setIndexEnable(1, true)
                } else {
                    control.currentType = ""
                    control.currentProfile = {}
                    dropDownView.setIndexEnable(1, false)
                }
            }
            onModelChanged: Qt.callLater(function() {
                if (count) {
                    var last = SystemHelper.loadSettings(control.myClassName + "/lastType", "")
                    if (last) {
                        for (var i = 0; i < count; i++) {
                            if (model[i] === last) {
                                currentIndex = i
                                return
                            }
                        }
                    }
                }
                currentIndex = -1
            })
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
            ScrollIndicator.vertical: ScrollIndicator { active: true }
            onCurrentIndexChanged: {
                if (~currentIndex) {
                    control.currentProfile = model[currentIndex]
                    dropDownView.setIndexEnable(2, true, false)
                } else {
                    control.currentProfile = {}
                    dropDownView.setIndexEnable(2, false)
                }
            }
            onModelChanged: Qt.callLater(function() {
                if (count) {
                    var last = SystemHelper.loadSettings(control.myClassName + "/lastProfile", "")
                    if (last) {
                        for (var i = 0; i < count; i++) {
                            if (model[i]["profile"] === last) {
                                currentIndex = i
                                return
                            }
                        }
                    }
                }
                currentIndex = -1
            })
        }
    }

    // Properties

    TextMetrics {
        id: propTextMetrics
        font: control.font
        text: "WWWWWWWWWW"
    }
    Component {
        id: propertiesComponent
        ListView {
            clip: true
            implicitWidth: control.availableWidth
            implicitHeight: Math.min(count, 10) * (propTextMetrics.height + 4)
            model: Object.keys(control.currentProfile)
            currentIndex: -1
            delegate: Row {
                padding: 2
                height: propTextMetrics.height + 4 // double Row padding
                Text {
                    width: propTextMetrics.advanceWidth
                    font: propTextMetrics.font
                    text: modelData
                    color: Material.accent
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: parent.ListView.view.width - propTextMetrics.advanceWidth - 4 // double Row padding
                    font: control.font
                    text: control.currentProfile[modelData]
                    color: Material.foreground
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
            ScrollIndicator.vertical: ScrollIndicator { active: true }
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
        id: dropDownPane
        show: currentFolder && currentProfile.hasOwnProperty("profile")
        RowLayout {
            anchors.fill: parent
            SquareButton {
                text: qsTr("Server")
                icon.source: "qrc:/icon-page-back"
                ToolTip.text: qsTr("Set cloud server")
                onClicked: appStackView.pop()
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                font.pointSize: appTitleSize
                text: dropDownPane.show ? currentProfile["profile"] : ""
                elide: Text.ElideRight
            }
            SquareButton {
                highlighted: true
                text: qsTr("Config")
                icon.source: "qrc:/icon-page-next"
                ToolTip.text: qsTr("VM configuration")
                onClicked: {
                    if (!SystemHelper.saveObject(currentFolder + "/lastProfile", Object.assign(currentProfile, lastServer))) {
                        appError(qsTr("Can't save current configuration"))
                        return
                    }
                    appPage("VMConfigPage.qml", { "currentFolder": currentFolder })
                }
            }
        }
    }
}
