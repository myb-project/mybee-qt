import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("Preferences")

    property int startIndex: 0

    Component.onCompleted: {
        if (startIndex > 0 && startIndex < tabBar.count)
            tabBar.setCurrentIndex(startIndex)
    }

    component TabButtonTemplate: TabButton {
        focusPolicy: Qt.NoFocus
        width: Math.floor(tabBar.width / tabBar.count)
        down: checked
    }

    header: TabBar {
        id: tabBar
        spacing: 0
        currentIndex: swipeView.currentIndex
        TabButtonTemplate {
            text: Qt.application.displayName
        }
        TabButtonTemplate {
            text: VMConfigSet.cbsdName.toUpperCase()
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        AppConfigTab {
            id: appConfigTab
        }
        CbsdConfigTab {
            id: cbsdConfigTab
        }
    }

    footer: DropDownPane {
        RowLayout {
            anchors.fill: parent
            SquareButton {
                text: qsTr("Reset")
                icon.source: "qrc:/icon-reset"
                ToolTip.text: qsTr("Reset to defaults")
                onClicked: {
                    appWarning(qsTr("Reset all settings to default values?"), Dialog.Yes | Dialog.No)
                    .accepted.connect(function() {
                        if (appWindow.visibility === ApplicationWindow.Windowed) {
                            appWindow.width = appFitWidth
                            appWindow.height = appFitHeight
                            if (Screen.primaryOrientation === Qt.LandscapeOrientation)
                                appDelay(appTinyDelay, appCompactAction.toggle)
                        }
                        appMaterialTheme = MaterialSet.defaultTheme
                        appMainFont = defaultMainFont
                        appMonoFont = defaultMonoFont
                        appResetFont(defaultMainFont, Math.round(Qt.application.font.pointSize))

                        appShowDebug = true
                        SystemHelper.saveSettings("showDebug", appShowDebug)

                        SystemHelper.saveSettings(SshProcess.portNumberKey());
                        SystemHelper.saveSettings(SshProcess.timeoutSecKey());

                        VMConfigSet.setDefaults()

                        appStackView.pop()
                    })
                }
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                font.pointSize: appTitleSize
                text: saveButton.enabled ? qsTr("Save changes?") : ""
                elide: Text.ElideLeft
            }
            SquareButton {
                id: saveButton
                enabled: appConfigTab.modified || cbsdConfigTab.modified
                highlighted: enabled
                text: qsTr("Save")
                icon.source: "qrc:/icon-ok"
                ToolTip.text: qsTr("Save changes")
                onClicked: {
                    appMaterialTheme = appConfigTab.colorTheme
                    appMainFont = appConfigTab.mainFontName
                    appMonoFont = appConfigTab.monoFontName
                    appResetFont(appConfigTab.mainFontName, appConfigTab.fontPointSize)

                    if (appConfigTab.showDebug !== appShowDebug) {
                        appShowDebug = appConfigTab.showDebug
                        SystemHelper.saveSettings("showDebug", appShowDebug)
                    }

                    SystemHelper.saveSettings(SshProcess.portNumberKey(), appConfigTab.sshPortNumber);
                    SystemHelper.saveSettings(SshProcess.timeoutSecKey(), appConfigTab.sshTimeoutSec);

                    VMConfigSet.clusterPeriod = appConfigTab.clusterTimer
                    VMConfigSet.progressPeriod = appConfigTab.progressTimer

                    VMConfigSet.cbsdSshExec = cbsdConfigTab.cbsdSshExec
                    VMConfigSet.cbsdCapabilities = cbsdConfigTab.cbsdCapabilities
                    VMConfigSet.cbsdProfiles = cbsdConfigTab.cbsdProfiles
                    VMConfigSet.cbsdCluster = cbsdConfigTab.cbsdCluster
                    VMConfigSet.cbsdCreate = cbsdConfigTab.cbsdCreate
                    VMConfigSet.cbsdStart = cbsdConfigTab.cbsdStart
                    VMConfigSet.cbsdStop = cbsdConfigTab.cbsdStop
                    VMConfigSet.cbsdDestroy = cbsdConfigTab.cbsdDestroy

                    appStackView.pop()
                }
            }
        }
    }
}
