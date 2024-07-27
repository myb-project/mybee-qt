import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("Preferences")

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    property bool modified: false

    Component.onCompleted: {
        dropDownView.setIndexEnable(0, true)
        dropDownView.setIndexEnable(1, true, false)
        dropDownView.setIndexEnable(2, true, false)
    }

    readonly property var dropDownModel: [
        {   // index 0
            icon: "qrc:/icon-appearance",
            text: QT_TR_NOOP("Appearance"),
            list: [ { text: QT_TR_NOOP("Color theme"), item: colorThemeComponent },
                    { text: QT_TR_NOOP("Font size"), item: fontSizeComponent },
                    { text: QT_TR_NOOP("Language"), item: languageComponent },
                    { text: QT_TR_NOOP("Show debug"), item: showDebugComponent } ]
        },{ // index 1
            icon: "qrc:/icon-connection",
            text: QT_TR_NOOP("Connection"),
            list: [ { text: QT_TR_NOOP("SSH port"), item: sshPortComponent },
                    { text: QT_TR_NOOP("SSH timeout"), item: sshTimeoutComponent },
                    { text: QT_TR_NOOP("Cluster poll"), item: clusterTimerComponent },
                    { text: QT_TR_NOOP("Progress poll"), item: progressTimerComponent } ]
        },{ // index 2
            icon: "qrc:/icon-local-db",
            text: qsTr("%1 parameters").arg(VMConfigSet.cbsdName.toUpperCase()),
            list: [ { text: QT_TR_NOOP("Local install"), item: cbsdLocalComponent },
                    { text: QT_TR_NOOP("SSH execute"), item: cbsdSshComponent },
                    { text: QT_TR_NOOP("Get profiles"), item: cbsdProfilesComponent },
                    { text: QT_TR_NOOP("Get cluster"), item: cbsdClusterComponent },
                    { text: QT_TR_NOOP("VM create"), item: cbsdCreateComponent },
                    { text: QT_TR_NOOP("VM start"), item: cbsdStartComponent },
                    { text: QT_TR_NOOP("VM stop"), item: cbsdStopComponent },
                    { text: QT_TR_NOOP("VM destroy"), item: cbsdDestroyComponent } ]
        }
    ]

    Component {
        id: headerComponent
        Pane {
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-preferences"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://www.bsdstore.ru")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Customize your preferences")
                }
            }
        }
    }

    // Appearance

    property int colorTheme: appMaterialTheme
    Component {
        id: colorThemeComponent
        RowLayout {
            RadioButton {
                text: qsTr("Dark")
                checked: control.colorTheme === Material.Dark
                onToggled: {
                    if (checked) {
                        control.colorTheme = Material.Dark
                        control.modified = true
                    }
                }
            }
            RadioButton {
                text: qsTr("Light")
                checked: control.colorTheme === Material.Light
                onToggled: {
                    if (checked) {
                        control.colorTheme = Material.Light
                        control.modified = true
                    }
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    TextMetrics {
        id: sliderTextMetrics
        font: control.font
        text: "000"
    }
    property real fontPointSize: appWindow.font.pointSize
    Component {
        id: fontSizeComponent
        RowLayout {
            Slider {
                id: slider
                Layout.fillWidth: true
                from: appOrigFontSize - (appOrigFontSize >= 12 ? 4 : 2)
                to: appOrigFontSize + (appOrigFontSize >= 12 ? 4 : 2)
                stepSize: value >= 12 ? 2 : 1
                snapMode: Slider.SnapAlways
                value: appWindow.font.pointSize
                onMoved: {
                    control.fontPointSize = value
                    control.modified = true
                }
            }
            Label {
                Layout.preferredWidth: sliderTextMetrics.tightBoundingRect.width
                horizontalAlignment: Text.AlignRight
                font.pointSize: slider.value
                text: Math.round(slider.value)
            }
        }
    }

    Component { //XXX No implemented yet!
        id: languageComponent
        ComboBox {
            enabled: false
            model: Qt.locale().uiLanguages
        }
    }

    property bool showDebug: appShowDebug
    Component {
        id: showDebugComponent
        Switch {
            enabled: !SystemHelper.envVariable("MYBEE_QT_DEBUG")
            checked: appShowDebug
            text: checked ? qsTr("Enabled") : qsTr("Disabled")
            onToggled: {
                control.showDebug = checked
                control.modified = true
            }
        }
    }

    // Connection

    property int sshPortNumber: SystemHelper.loadSettings(SshProcess.portNumberKey(), SshProcess.defaultPortNumber())
    Component {
        id: sshPortComponent
        SpinBox {
            editable: true
            from: SshProcess.defaultPortNumber(); to: 65535; stepSize: 1
            textFromValue: function(value) { return qsTr("%1 / tcp").arg(value) }
            onValueModified: {
                control.sshPortNumber = value
                control.modified = true
            }
            Component.onCompleted: value = control.sshPortNumber
        }
    }

    property int sshTimeoutSec: SystemHelper.loadSettings(SshProcess.timeoutSecKey(), SshProcess.defaultTimeoutSec())
    Component {
        id: sshTimeoutComponent
        RowLayout {
            Slider {
                id: slider
                Layout.fillWidth: true
                from: SshProcess.defaultTimeoutSec(); to: from * 3; stepSize: 1
                snapMode: Slider.SnapAlways
                onMoved: {
                    control.sshTimeoutSec = value
                    control.modified = true
                }
                Component.onCompleted: value = control.sshTimeoutSec
            }
            Label {
                Layout.preferredWidth: sliderTextMetrics.tightBoundingRect.width
                horizontalAlignment: Text.AlignRight
                text: slider.value + " s"
            }
        }
    }

    property int clusterTimer: VMConfigSet.clusterPeriod
    Component {
        id: clusterTimerComponent
        SpinBox {
            from: 30; to: 180; stepSize: 10
            textFromValue: function(value) { return qsTr("Every %1 sec").arg(value) }
            value: VMConfigSet.clusterPeriod
            onValueModified: {
                control.clusterTimer = value
                control.modified = true
            }
        }
    }

    property int progressTimer: VMConfigSet.progressPeriod
    Component {
        id: progressTimerComponent
        SpinBox {
            from: 1; to: 5; stepSize: 1
            textFromValue: function(value) { return qsTr("Every %1 sec").arg(value) }
            value: VMConfigSet.progressPeriod
            onValueModified: {
                control.progressTimer = value
                control.modified = true
            }
        }
    }

    // CBSD parameters

    Component {
        id: cbsdLocalComponent
        MyTextField {
            readOnly: true
            text: VMConfigSet.cbsdPath ? VMConfigSet.sudoExec + ' ' + VMConfigSet.cbsdPath : qsTr("Not found")
        }
    }

    property string cbsdSshExec: VMConfigSet.cbsdSshExec
    Component {
        id: cbsdSshComponent
        MyTextField {
            text: VMConfigSet.cbsdSshExec
            onEditingFinished: {
                control.cbsdSshExec = text
                control.modified = true
            }
        }
    }

    property string cbsdProfiles: VMConfigSet.cbsdProfiles
    Component {
        id: cbsdProfilesComponent
        MyTextField {
            text: VMConfigSet.cbsdProfiles
            onEditingFinished: {
                control.cbsdProfiles = text
                control.modified = true
            }
        }
    }

    property string cbsdCluster: VMConfigSet.cbsdCluster
    Component {
        id: cbsdClusterComponent
        MyTextField {
            text: VMConfigSet.cbsdCluster
            onEditingFinished: {
                control.cbsdCluster = text
                control.modified = true
            }
        }
    }

    property string cbsdCreate: VMConfigSet.cbsdCreate
    Component {
        id: cbsdCreateComponent
        MyTextField {
            text: VMConfigSet.cbsdCreate
            onEditingFinished: {
                control.cbsdCreate = text
                control.modified = true
            }
        }
    }

    property string cbsdStart: VMConfigSet.cbsdStart
    Component {
        id: cbsdStartComponent
        MyTextField {
            text: VMConfigSet.cbsdStart
            onEditingFinished: {
                control.cbsdStart = text
                control.modified = true
            }
        }
    }

    property string cbsdStop: VMConfigSet.cbsdStop
    Component {
        id: cbsdStopComponent
        MyTextField {
            text: VMConfigSet.cbsdStop
            onEditingFinished: {
                control.cbsdStop = text
                control.modified = true
            }
        }
    }

    property string cbsdDestroy: VMConfigSet.cbsdDestroy
    Component {
        id: cbsdDestroyComponent
        MyTextField {
            text: VMConfigSet.cbsdDestroy
            onEditingFinished: {
                control.cbsdDestroy = text
                control.modified = true
            }
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
        RowLayout {
            anchors.fill: parent
            SquareButton {
                text: qsTr("Reset")
                icon.source: "qrc:/icon-reset"
                ToolTip.text: qsTr("Reset to defaults")
                onClicked: {
                    if (appWindow.visibility === ApplicationWindow.Windowed) {
                        appWindow.width = appFitWidth
                        appWindow.height = appFitHeight
                    }
                    appMaterialTheme = MaterialSet.defaultTheme
                    appWindow.font.pointSize = appOrigFontSize ? appOrigFontSize : 9.75

                    appShowDebug = true
                    SystemHelper.saveSettings("showDebug", appShowDebug)

                    SystemHelper.saveSettings(SshProcess.portNumberKey());
                    SystemHelper.saveSettings(SshProcess.timeoutSecKey());

                    VMConfigSet.setDefaults()

                    appStackView.pop()
                }
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                font.pointSize: appTitleSize
                text: appSettingsPath
                elide: Text.ElideLeft
            }
            SquareButton {
                enabled: control.modified
                highlighted: enabled
                text: qsTr("Save")
                icon.source: "qrc:/icon-ok"
                ToolTip.text: qsTr("Save changes")
                onClicked: {
                    appMaterialTheme = control.colorTheme
                    appWindow.font.pointSize = control.fontPointSize
                    if (control.showDebug !== appShowDebug) {
                        appShowDebug = control.showDebug
                        SystemHelper.saveSettings("showDebug", appShowDebug)
                    }

                    SystemHelper.saveSettings(SshProcess.portNumberKey(), control.sshPortNumber);
                    SystemHelper.saveSettings(SshProcess.timeoutSecKey(), control.sshTimeoutSec);

                    VMConfigSet.clusterPeriod = control.clusterTimer
                    VMConfigSet.progressPeriod = control.progressTimer

                    VMConfigSet.cbsdSshExec = control.cbsdSshExec
                    VMConfigSet.cbsdProfiles = control.cbsdProfiles
                    VMConfigSet.cbsdCluster = control.cbsdCluster
                    VMConfigSet.cbsdCreate = control.cbsdCreate
                    VMConfigSet.cbsdStart = control.cbsdStart
                    VMConfigSet.cbsdStop = control.cbsdStop
                    VMConfigSet.cbsdDestroy = control.cbsdDestroy

                    appStackView.pop()
                }
            }
        }
    }
}
