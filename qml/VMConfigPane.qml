import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Pane {
    id: control
    padding: 0

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    readonly property double gigaByte: 1024 * 1024 * 1024
    property var vmConfig: ({})
    property bool modified: false
    readonly property bool isValid: vmConfig.hasOwnProperty("server") && vmConfig.hasOwnProperty("alias")
    readonly property bool isCreated: vmConfig.hasOwnProperty("id")
    readonly property bool isSshHost: vmConfig.hasOwnProperty("ssh_host")
    readonly property bool isRdpHost: vmConfig.hasOwnProperty("rdp_host")
    readonly property bool isVncHost: vmConfig.hasOwnProperty("vnc_host")

    //readonly property int vmProfileIndex: vmSettings["name"] ? vmProfiles.findIndex(obj => obj.name === vmSettings["name"]) : -1

    Component.onCompleted: {
        dropDownView.setIndexEnable(0, true)
        dropDownView.setIndexEnable(1, true, !isCreated)
        dropDownView.setIndexEnable(2, isSshHost || isRdpHost || isVncHost)
        dropDownView.setIndexEnable(3, true, false)
    }

    readonly property var dropDownModel: [
        {   // index 0
            icon: "",
            text: QT_TR_NOOP("Title"),
            list: [ { text: QT_TR_NOOP("Name"), item: nameComponent },
                    { text: QT_TR_NOOP("Alias"), item: aliasComponent } ]
        },{ // index 1
            icon: "",
            text: QT_TR_NOOP("Equipment"),
            list: [ { text: QT_TR_NOOP("Disk, GB"), item: diskSizeComponent },
                    { text: QT_TR_NOOP("RAM, GB"), item: ramSizeComponent },
                    { text: QT_TR_NOOP("CPU count"), item: cpuCountComponent } ]
        },{ // index 2
            icon: "",
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

    function setVarValue(varName, value) {
        //console.debug("setVarValue", varName, value)
        if (vmConfig[varName] !== value) {
            var obj = vmConfig
            if (value !== undefined && value !== "") obj[varName] = value
            else if (obj.hasOwnProperty(varName)) delete obj[varName]
            else return
            vmConfig = Object.assign({}, obj)
            modified = true
        }
    }

    // Title

    Component {
        id: nameComponent
        MyTextField {
            text: vmConfig.hasOwnProperty("name") ? vmConfig["name"] : ""
            onEditingFinished: setVarValue("name", text)
        }
    }

    Component {
        id: aliasComponent
        MyTextField {
            text: vmConfig.hasOwnProperty("alias") ? vmConfig["alias"] : ""
            onEditingFinished: setVarValue("alias", text)
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
        signal curModified()
        Slider {
            id: slider
            Layout.fillWidth: true
            focusPolicy: Qt.NoFocus
            stepSize: 1
            snapMode: Slider.SnapAlways
            onPressedChanged: {
                if (!pressed) parent.curModified()
            }
        }
        MyTextField {
            Layout.preferredWidth: leftPadding + sliderTextMetrics.advanceWidth + rightPadding
            validator: IntValidator { bottom: slider.from; top: slider.to }
            text: slider.value
            onEditingFinished: {
                slider.value = text
                parent.curModified()
            }
        }
    }

    Component {
        id: diskSizeComponent
        SliderTemplate {
            property int disk: vmConfig.hasOwnProperty("imgsize_bytes") ? Math.round(vmConfig["imgsize_bytes"] / control.gigaByte) : 0
            min: vmConfig.hasOwnProperty("imgsize_min_bytes") ? Math.round(vmConfig["imgsize_min_bytes"] / control.gigaByte) : 1
            max: vmConfig.hasOwnProperty("imgsize_max_bytes") ? Math.round(vmConfig["imgsize_max_bytes"] / control.gigaByte)
                                                              : sliderTextMetrics.text
            cur: disk ? disk : RestApiSet.defDiskSize
            onCurModified: setVarValue("imgsize_bytes", control.gigaByte * cur)
        }
    }

    Component {
        id: ramSizeComponent
        SliderTemplate {
            property int ram: vmConfig.hasOwnProperty("ram_bytes") ? Math.round(vmConfig["ram_bytes"] / control.gigaByte) : 0
            min: vmConfig.hasOwnProperty("vm_ram_min_bytes") ? Math.round(vmConfig["vm_ram_min_bytes"] / control.gigaByte) : 1
            max: vmConfig.hasOwnProperty("vm_ram_max_bytes") ? Math.round(vmConfig["vm_ram_max_bytes"] / control.gigaByte) : 64
            cur: ram ? ram : RestApiSet.defRamSize
            onCurModified: setVarValue("ram_bytes", control.gigaByte * cur)
        }
    }

    Component {
        id: cpuCountComponent
        SliderTemplate {
            property int cpu: vmConfig.hasOwnProperty("cpus") ? vmConfig["cpus"] : 0
            min: vmConfig.hasOwnProperty("vm_cpus_min") ? vmConfig["vm_cpus_min"] : 1
            max: vmConfig.hasOwnProperty("vm_cpus_max") ? vmConfig["vm_cpus_max"] : 16
            cur: cpu ? cpu : RestApiSet.defCpuCount
            onCurModified: setVarValue("cpus", cur)
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
            text: vmConfig.hasOwnProperty("ssh_term") ? vmConfig["ssh_term"] : "xterm-256color"
            onEditingFinished: setVarValue("ssh_term", text)
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
                onToggled: if (checked) setVarValue("desktop", "rdp")
            }
            RadioButtonTemplate {
                id: vncButton
                enabled: control.isVncHost
                text: qsTr("VNC sharing")
                tipText: qsTr("Virtual Network Computing")
                onToggled: if (checked) setVarValue("desktop", "vnc")
            }
            Item { Layout.fillWidth: true }
            Component.onCompleted: {
                var desktop = vmConfig.hasOwnProperty("desktop") ? vmConfig["desktop"] : ""
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
                onToggled: if (checked) setVarValue("quality", "fast")
            }
            RadioButtonTemplate {
                id: aveButton
                text: qsTr("Ave")
                tipText: qsTr("Optimal average")
                onToggled: if (checked) setVarValue("quality", "ave")
            }
            RadioButtonTemplate {
                id: bestButton
                text: qsTr("LAN")
                tipText: qsTr("Best quality but slow transfer")
                onToggled: if (checked) setVarValue("quality", "best")
            }
            Item { Layout.fillWidth: true }
            Component.onCompleted: {
                //if (!control.isRdpHost && !control.isVncHost) return
                var quality = vmConfig.hasOwnProperty("quality") ? vmConfig["quality"] : ""
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
            model: Object.keys(vmConfig)
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
                    text: vmConfig[modelData]
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
}
