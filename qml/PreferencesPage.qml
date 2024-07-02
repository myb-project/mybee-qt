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
        if (VMConfigSet.cbsdPath) {
            dropDownView.setIndexEnable(1, true, false)
            dropDownView.setIndexEnable(2, true, true)
        } else {
            dropDownView.setIndexEnable(1, true, true)
            dropDownView.setIndexEnable(2, false)
        }
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
            list: [ { text: QT_TR_NOOP("Transport"), item: transportComponent },
                    { text: QT_TR_NOOP("Cluster poll"), item: clusterTimerComponent },
                    { text: QT_TR_NOOP("Progress poll"), item: progressTimerComponent },
                    { text: QT_TR_NOOP("SSH host key"), item: serverKeyComponent } ]
        },{ // index 2
            icon: "qrc:/icon-local-db",
            text: VMConfigSet.cbsdPath ? VMConfigSet.cbsdPath : VMConfigSet.cbsdName,
            list: [ { text: QT_TR_NOOP("Get profiles"), item: cbsdProfilesComponent },
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
                RoundButton {
                    icon.source: "qrc:/icon-reset"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Reset to defaults")
                    ToolTip.delay: appTipDelay
                    ToolTip.timeout: appTipTimeout
                    onClicked: {
                        if (appWindow.visibility === ApplicationWindow.Windowed) {
                            appWindow.width = appFitWidth
                            appWindow.height = appFitHeight
                        }
                        appMaterialTheme = MaterialSet.defaultTheme
                        appWindow.font.pointSize = appOrigFontSize ? appOrigFontSize : 9.75

                        appShowDebug = true
                        SystemHelper.saveSettings("showDebug", appShowDebug)

                        RestApiSet.urlScheme = "http"
                        SystemHelper.saveSettings("urlScheme", RestApiSet.urlScheme)

                        VMConfigSet.clusterPeriod = 60
                        VMConfigSet.progressPeriod = 3
                        SystemHelper.saveSettings("checkServerKey", false)
                    }
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

    property string urlScheme: RestApiSet.urlScheme
    Component {
        id: transportComponent
        RowLayout {
            enabled: !SystemHelper.envVariable("CLOUD_URL")
            RadioButton {
                text: qsTr("HTTP")
                checked: control.urlScheme === "http"
                onToggled: {
                    if (checked) {
                        control.urlScheme = "http"
                        control.modified = true
                    }
                }
            }
            RadioButton {
                enabled: HttpRequest.sslAvailable
                text: qsTr("HTTPS")
                checked: control.urlScheme === "https"
                onToggled: {
                    if (checked) {
                        control.urlScheme = "https"
                        control.modified = true
                    }
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    property int clusterTimer: VMConfigSet.clusterPeriod
    Component {
        id: clusterTimerComponent
        RowLayout {
            Slider {
                id: slider
                Layout.fillWidth: true
                from: 30; to: 180; stepSize: 10
                snapMode: Slider.SnapAlways
                value: VMConfigSet.clusterPeriod
                onMoved: {
                    control.clusterTimer = value
                    control.modified = true
                }
            }
            Label {
                Layout.preferredWidth: sliderTextMetrics.tightBoundingRect.width
                horizontalAlignment: Text.AlignRight
                text: slider.value
            }
        }
    }

    property int progressTimer: VMConfigSet.progressPeriod
    Component {
        id: progressTimerComponent
        RowLayout {
            Slider {
                id: slider
                Layout.fillWidth: true
                from: 1; to: 5; stepSize: 1
                snapMode: Slider.SnapAlways
                value: VMConfigSet.progressPeriod
                onMoved: {
                    control.progressTimer = value
                    control.modified = true
                }
            }
            Label {
                Layout.preferredWidth: sliderTextMetrics.tightBoundingRect.width
                horizontalAlignment: Text.AlignRight
                text: slider.value
            }
        }
    }

    property bool checkServerKey: SystemHelper.loadSettings("checkServerKey", false)
    Component {
        id: serverKeyComponent
        RowLayout {
            RadioButton {
                text: qsTr("On trust")
                checked: !control.checkServerKey
                onToggled: {
                    if (checked) {
                        control.checkServerKey = false
                        control.modified = true
                    }
                }
            }
            RadioButton {
                text: qsTr("Strict check")
                checked: control.checkServerKey
                onToggled: {
                    if (checked) {
                        control.checkServerKey = true
                        control.modified = true
                    }
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    // CBSD mode options

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
        id: dropDownPane
        show: control.modified
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Save preferences?")
                elide: Text.ElideRight
            }
            DialogButtonBox {
                position: DialogButtonBox.Footer
                standardButtons: DialogButtonBox.Ok
                onAccepted: {
                    // prevent blames about binding loop detected for property "implicitHeight"
                    dropDownView.setIndexEnable(0, false)
                    dropDownView.setIndexEnable(1, false)
                    dropDownView.setIndexEnable(2, false)
                    dropDownPane.show = false

                    appMaterialTheme = control.colorTheme
                    appWindow.font.pointSize = control.fontPointSize
                    if (control.showDebug !== appShowDebug) {
                        appShowDebug = control.showDebug
                        SystemHelper.saveSettings("showDebug", appShowDebug)
                    }

                    if (control.urlScheme !== RestApiSet.urlScheme) {
                        RestApiSet.urlScheme = control.urlScheme
                        SystemHelper.saveSettings("urlScheme", RestApiSet.urlScheme)
                    }
                    VMConfigSet.clusterPeriod = control.clusterTimer
                    VMConfigSet.progressPeriod = control.progressTimer
                    SystemHelper.saveSettings("checkServerKey", control.checkServerKey)

                    if (VMConfigSet.cbsdPath) {
                        VMConfigSet.cbsdProfiles = control.cbsdProfiles
                        VMConfigSet.cbsdCluster = control.cbsdCluster
                        VMConfigSet.cbsdCreate = control.cbsdCreate
                        VMConfigSet.cbsdStart = control.cbsdStart
                        VMConfigSet.cbsdStop = control.cbsdStop
                        VMConfigSet.cbsdDestroy = control.cbsdDestroy
                    }
                    appStackView.pop()
                }
            }
        }
    }
}
