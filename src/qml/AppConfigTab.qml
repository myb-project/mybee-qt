import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

DropDownView {
    id: control

    header: headerComponent
    model: control.dropDownModel
    myClassName: control.toString().match(/.+?(?=_)/)[0]

    property bool modified: false
    property var mainFontList: []
    property var monoFontList: []

    Component.onCompleted: {
        var i, list = SystemHelper.fileList(":/main/fonts")
        for (i = 0; i < list.length; i++) {
            mainFontList.push(list[i].slice(0, list[i].lastIndexOf('.')).replace('-', ' '))
        }
        if (appMainFont && !mainFontList.includes(appMainFont))
            mainFontList.unshift(appMainFont)

        list = SystemHelper.fileList(":/mono/fonts")
        for (i = 0; i < list.length; i++) {
            monoFontList.push(list[i].slice(0, list[i].lastIndexOf('.')).replace('-', ' '))
        }
        if (appMonoFont && !monoFontList.includes(appMonoFont))
            monoFontList.unshift(appMonoFont)

        setIndexEnable(0, true)
        setIndexEnable(1, true, false)
    }

    readonly property var dropDownModel: [
        {   // index 0
            icon: "qrc:/icon-appearance",
            text: QT_TR_NOOP("Appearance"),
            list: [ { text: QT_TR_NOOP("Color theme"), item: colorThemeComponent },
                    { text: QT_TR_NOOP("Font types"), item: fontTypesComponent },
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
        }
    ]

    Component {
        id: headerComponent
        Pane {
            width: control.width
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-preferences"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally(SystemHelper.appHomeUrl)
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

    component ComboBoxTemplate: ComboBox {
        contentItem: Text {
            id: contentText
            leftPadding: 10
            rightPadding: parent.indicator.width + parent.spacing
            text: parent.displayText
            font.family: appFontFamily(text)
            font.styleName: appFontStyle(text)
            font.pointSize: control.fontPointSize
            color: Material.foreground
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        delegate: ItemDelegate {
            width: parent.width
            contentItem: Text {
                text: modelData
                font.family: appFontFamily(text)
                font.styleName: appFontStyle(text)
                font.pointSize: control.fontPointSize
                color: text === contentText.text ? Material.accent : Material.foreground
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }

    property string mainFontName: appMainFont
    property string monoFontName: appMonoFont
    Component {
        id: fontTypesComponent
        GridLayout {
            flow: appPortraitView ? GridLayout.TopToBottom : GridLayout.LeftToRight
            RowLayout {
                ComboBoxTemplate {
                    Layout.fillWidth: true
                    model: control.mainFontList
                    onActivated: function(index) {
                        control.mainFontName = model[index]
                        control.modified = true
                    }
                    Component.onCompleted: currentIndex = find(appMainFont)
                }
            }
            RowLayout {
                ComboBoxTemplate {
                    Layout.fillWidth: true
                    model: control.monoFontList
                    onActivated: function(index) {
                        control.monoFontName = model[index]
                        control.modified = true
                    }
                    Component.onCompleted: currentIndex = find(appMonoFont)
                }
            }
        }
    }

    TextMetrics {
        id: sliderTextMetrics
        font: appWindow.font
        text: "000"
    }
    property int fontPointSize: appFontPointSize
    Component {
        id: fontSizeComponent
        RowLayout {
            Slider {
                id: slider
                Layout.fillWidth: true
                property real defaultSize: Qt.application.font.pointSize
                from: defaultSize - (defaultSize >= 12 ? 4 : 2)
                to: defaultSize + (defaultSize >= 12 ? 4 : 2)
                stepSize: value >= 12 ? 2 : 1
                snapMode: Slider.SnapAlways
                value: appFontPointSize
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
}
