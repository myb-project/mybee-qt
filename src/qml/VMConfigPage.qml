import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: isCreated ? qsTr("VM config") : qsTr("VM create")
    enabled: !VMConfigSet.isBusy

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    readonly property double gigaByte: 1024 * 1024 * 1024
    property string currentFolder
    property var currentProfile: ({})
    readonly property bool isCreated: currentProfile.hasOwnProperty("id")
    readonly property bool isSshHost: currentProfile.hasOwnProperty("ssh_host")
    readonly property bool isRdpHost: currentProfile.hasOwnProperty("rdp_host")
    readonly property bool isVncHost: currentProfile.hasOwnProperty("vnc_host")
    readonly property string profileName: currentProfile.hasOwnProperty("vm_os_profile") ? currentProfile["vm_os_profile"] :
                                            (currentProfile.hasOwnProperty("profile") ? currentProfile["profile"] : "")

    readonly property var dropDownModel: [
        {   // index 0
            icon: "qrc:/icon-cloud-instance",
            text: QT_TR_NOOP("Title"),
            list: [ { text: QT_TR_NOOP("Name"), item: nameComponent },
                    { text: QT_TR_NOOP("Alias"), item: aliasComponent } ]
        },{ // index 1
            icon: "qrc:/icon-equipment",
            text: QT_TR_NOOP("Equipment"),
            list: [ { text: QT_TR_NOOP("Disk, GB"), item: diskSizeComponent },
                    { text: QT_TR_NOOP("RAM, GB"), item: ramSizeComponent },
                    { text: QT_TR_NOOP("CPU count"), item: cpuCountComponent } ]
        },{ // index 2
            icon: "qrc:/icon-connection",
            text: QT_TR_NOOP("Remote access"),
            list: [ { text: QT_TR_NOOP("Terminal"), item: terminalComponent },
                    { text: QT_TR_NOOP("Desktop"), item: desktopComponent },
                    { text: QT_TR_NOOP("Quality"), item: qualityComponent } ]
        },{ // index 3
            icon: "",
            text: QT_TR_NOOP("Summary"),
            padding: 0,
            list: [ { item: summaryComponent } ]
        }
    ]

    Component.onCompleted: {
        if (currentFolder) {
            currentProfile = SystemHelper.loadObject(currentFolder + "/lastProfile")
            if (!currentProfile.hasOwnProperty("server") || !currentProfile.hasOwnProperty("ssh_key")) {
                appError(qsTr("Can't load current configuration"))
                return
            }
            var prefix = currentProfile["default_jname"] ? currentProfile["default_jname"] : currentProfile["type"]
            currentProfile["alias"] = VMConfigSet.uniqueValue("alias", prefix ? prefix : "vm")

            appDelay(appTinyDelay, VMConfigSet.getList,
                     { "server": currentProfile["server"], "ssh_key": currentProfile["ssh_key"] })
        } else {
            currentFolder = SystemHelper.fileName(VMConfigSet.valueAt("server"))
            if (!currentFolder) {
                appError(qsTr("Can't load current configuration"))
                return
            }
            currentProfile = Object.assign({}, VMConfigSet.currentConfig)
            setupView()
        }
    }

    function setupView() {
        var ra = isSshHost || isRdpHost || isVncHost
        dropDownView.setIndexEnable(0, true)
        dropDownView.setIndexEnable(1, true, !isCreated)
        dropDownView.setIndexEnable(2, ra, ra && isCreated)
        dropDownView.setIndexEnable(3, true, false)
    }

    Connections {
        target: VMConfigSet
        function onListDone(url) {
            if (url === currentProfile["server"]) setupView()
        }
    }

    Component {
        id: headerComponent
        Pane {
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: control.isCreated ? "qrc:/image-settings-ok" : "qrc:/image-settings"
                    text: qsTr("Visit Wikipedia")
                    onClicked: Qt.openUrlExternally("https://wikipedia.org/wiki/Virtual_machine")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: control.isCreated ?
                              qsTr("Configuring a virtial machine using the <i>%1</i> profile").arg(profileName) :
                              qsTr("Creating a new virtial machine using the <i>%1</i> profile").arg(profileName)
                }
            }
        }
    }

    // Title

    Component {
        id: nameComponent
        MyTextField {
            text: currentProfile.hasOwnProperty("name") ? currentProfile["name"] : ""
            onEditingFinished: currentProfile["name"] = text
        }
    }

    Component {
        id: aliasComponent
        MyTextField {
            text: currentProfile.hasOwnProperty("alias") ? currentProfile["alias"] : ""
            onTextChanged: aliasLabel.text = text
            onEditingFinished: currentProfile["alias"] = text
            Component.onCompleted: if (!isMobile && !control.isCreated) forceActiveFocus()
        }
    }

    // Equipment

    TextMetrics {
        id: sliderTextMetrics
        font: control.font
        text: "99999"
    }
    component SliderTemplate: RowLayout {
        enabled: !control.isCreated
        property alias min: slider.from
        property alias max: slider.to
        property alias cur: slider.value
        Slider {
            id: slider
            Layout.fillWidth: true
            stepSize: 1
            snapMode: Slider.SnapAlways
            onPressedChanged: if (!pressed) parent.setProfile()
        }
        MyTextField {
            Layout.preferredWidth: leftPadding + sliderTextMetrics.advanceWidth + rightPadding
            validator: IntValidator { bottom: slider.from; top: slider.to }
            text: slider.value
            onEditingFinished: {
                slider.value = text
                parent.setProfile()
            }
        }
        Component.onCompleted: setProfile()
    }

    Component {
        id: diskSizeComponent
        SliderTemplate {
            property int disk: currentProfile.hasOwnProperty("imgsize_bytes") ? Math.round(currentProfile["imgsize_bytes"] / control.gigaByte) : 0
            min: currentProfile.hasOwnProperty("imgsize_min_bytes") ? Math.round(currentProfile["imgsize_min_bytes"] / control.gigaByte) : 1
            max: currentProfile.hasOwnProperty("imgsize_max_bytes") ? Math.round(currentProfile["imgsize_max_bytes"] / control.gigaByte)
                                                                    : sliderTextMetrics.text
            cur: disk ? disk : RestApiSet.defDiskSize
            function setProfile() {
                currentProfile["imgsize"] = Math.round(cur) + 'g'
                currentProfile["imgsize_bytes"] = Math.round(control.gigaByte * cur)
            }
        }
    }

    Component {
        id: ramSizeComponent
        SliderTemplate {
            property int ram: currentProfile.hasOwnProperty("ram_bytes") ? Math.round(currentProfile["ram_bytes"] / control.gigaByte) : 0
            min: currentProfile.hasOwnProperty("vm_ram_min_bytes") ? Math.round(currentProfile["vm_ram_min_bytes"] / control.gigaByte) : 1
            max: currentProfile.hasOwnProperty("vm_ram_max_bytes") ? Math.round(currentProfile["vm_ram_max_bytes"] / control.gigaByte) : 64
            cur: ram ? ram : RestApiSet.defRamSize
            function setProfile() {
                currentProfile["ram"] = Math.round(cur) + 'g'
                currentProfile["ram_bytes"] = Math.round(control.gigaByte * cur)
            }
        }
    }

    Component {
        id: cpuCountComponent
        SliderTemplate {
            property int cpu: currentProfile.hasOwnProperty("cpus") ? currentProfile["cpus"] : 0
            min: currentProfile.hasOwnProperty("vm_cpus_min") ? currentProfile["vm_cpus_min"] : 1
            max: currentProfile.hasOwnProperty("vm_cpus_max") ? currentProfile["vm_cpus_max"] : 16
            cur: cpu ? cpu : RestApiSet.defCpuCount
            function setProfile() {
                currentProfile["cpus"] = Math.round(cur)
            }
        }
    }

    // Remote access

    component RadioButtonTemplate : RadioButton {
        required property string tipText
        ToolTip.visible: hovered
        ToolTip.text: tipText
        ToolTip.delay: appTipDelay
        ToolTip.timeout: appTipTimeout
    }

    Component {
        id: terminalComponent
        MyTextField {
            enabled: control.isSshHost
            text: currentProfile.hasOwnProperty("ssh_term") ? currentProfile["ssh_term"] : "xterm-256color"
            onEditingFinished: currentProfile["ssh_term"] = text
        }
    }

    Component {
        id: desktopComponent
        RowLayout {
            RadioButtonTemplate {
                id: rdpButton
                enabled: control.isRdpHost
                text: qsTr("RDP session")
                tipText: qsTr("Remote Desktop Protocol")
                onToggled: if (checked) currentProfile["desktop"] = "rdp"
            }
            RadioButtonTemplate {
                id: vncButton
                enabled: control.isVncHost
                text: qsTr("VNC sharing")
                tipText: qsTr("Virtual Network Computing")
                onToggled: if (checked) currentProfile["desktop"] = "vnc"
            }
            Item { Layout.fillWidth: true }
            Component.onCompleted: {
                var desktop = currentProfile.hasOwnProperty("desktop") ? currentProfile["desktop"] : ""
                if (control.isRdpHost && (!desktop || desktop === "rdp")) rdpButton.checked = true
                else if (control.isVncHost && (!desktop || desktop === "vnc")) vncButton.checked = true
            }
        }
    }

    Component {
        id: qualityComponent
        RowLayout {
            enabled: control.isRdpHost || control.isVncHost
            RadioButtonTemplate {
                id: fastButton
                text: qsTr("WAN")
                tipText: qsTr("Fast transfer but poor quality")
                onToggled: if (checked) currentProfile["quality"] = "fast"
            }
            RadioButtonTemplate {
                id: aveButton
                text: qsTr("Ave")
                tipText: qsTr("Optimal average")
                onToggled: if (checked) currentProfile["quality"] = "ave"
            }
            RadioButtonTemplate {
                id: bestButton
                text: qsTr("LAN")
                tipText: qsTr("Best quality but slow transfer")
                onToggled: if (checked) currentProfile["quality"] = "best"
            }
            Item { Layout.fillWidth: true }
            Component.onCompleted: {
                //if (!control.isRdpHost && !control.isVncHost) return
                var quality = currentProfile.hasOwnProperty("quality") ? currentProfile["quality"] : ""
                if (quality === "fast") fastButton.checked = true
                else if (quality === "best") bestButton.checked = true
                else aveButton.checked = true
            }
        }
    }

    // Summary

    TextMetrics {
        id: textMetrics
        font: control.font
        text: "WWWWWWWWWW"
    }
    Component {
        id: summaryComponent
        ListView {
            clip: true
            implicitWidth: control.availableWidth
            implicitHeight: Math.min(count, 10) * (textMetrics.height + 4)
            model: Object.keys(currentProfile)
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
                    text: currentProfile[modelData]
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

    Label {
        anchors.centerIn: parent
        visible: !control.isCreated
        font.pointSize: appWindow.font.pointSize * 6
        font.bold: true
        rotation: 45
        opacity: 0.1
        text: qsTr("Template")
    }

    footer: DropDownPane {
        show: currentFolder && aliasLabel.text
        RowLayout {
            anchors.fill: parent
            SquareButton {
                visible: !control.isCreated
                text: qsTr("Profile")
                icon.source: "qrc:/icon-page-back"
                ToolTip.text: qsTr("Get VM profiles")
                onClicked: appStackView.pop()
            }
            SquareButton {
                visible: control.isCreated
                enabled: VMConfigSet.isValid
                text: qsTr("Remove")
                icon.source: "qrc:/icon-remove"
                ToolTip.text: qsTr("Remove VM %1").arg(VMConfigSet.valueAt("alias"))
                onClicked: {
                    appWarning(qsTr("Do you want to remove %1?").arg(VMConfigSet.valueAt("alias")),
                               Dialog.Yes | Dialog.No).accepted.connect(function() {
                                   VMConfigSet.removeVm()
                                   appStackView.pop()
                               })
                }
            }
            Label {
                id: aliasLabel
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                font.pointSize: appTitleSize
                elide: Text.ElideRight
            }
            SquareButton {
                highlighted: true
                text: control.isCreated ? qsTr("Save") : qsTr("Create")
                icon.source: "qrc:/icon-ok"
                ToolTip.text: control.isCreated ? qsTr("Save changes") : qsTr("Create a new VM")
                onClicked: {
                    if (!currentProfile["name"] || !currentProfile["alias"]) {
                        appInfo(qsTr("Please specify the name and alias of the VM"))
                        return
                    }
                    if (!control.isCreated) {
                        if (!VMConfigSet.isUniqueValue("alias", currentProfile["alias"])) {
                            appInfo(qsTr("The alias %1 already exist\n\nPlease choose another alias").arg(currentProfile["alias"]))
                            return
                        }
                    }
                    if (!VMConfigSet.updateCurrent(currentProfile)) {
                        appError(qsTr("Can't save current configuration"))
                        return
                    }
                    if (!control.isCreated) {
                        if (!SystemHelper.saveObject(currentFolder + "/lastCreate", currentProfile)) {
                            appError(qsTr("Can't save current configuration"))
                            return
                        }
                        appDelay(appTinyDelay, VMConfigSet.createVm, currentProfile)
                    }
                    if (control.isCreated) appStackView.pop()
                    else appStackView.pop(null)
                }
            }
        }
    }
}
