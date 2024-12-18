import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Page {
    id: control
    title: qsTr("Wipe Files")

    readonly property int iconSize: appRowHeight

    property int cacheReqId: 0
    property real cacheSize: 0.0

    property int configReqId: 0
    property real configSize: 0.0

    property int dataReqId: 0
    property real dataSize: 0.0

    function prettyBytes(bytes) : string {
        if (bytes >= 1000000000) return (bytes / (1024 * 1024 * 1024)).toFixed(1) + qsTr(" GB")
        if (bytes >= 1000000) return (bytes / (1024 * 1024)).toFixed(1) + qsTr(" MB")
        if (bytes >= 1024) return (bytes / 1024).toFixed(1) + qsTr(" KB")
        return bytes
    }
    function prettyMBytes(mbytes) : string {
        if (mbytes >= 1024) return (mbytes / 1024).toFixed(1) + qsTr(" GB")
        return mbytes + qsTr(" MB")
    }

    Component.onCompleted: appDelay(appTinyDelay, function() {
        cacheReqId = dirSpaceUsed.start(SystemHelper.appCacheDir)
        configReqId = dirSpaceUsed.start(SystemHelper.appConfigDir)
        dataReqId = dirSpaceUsed.start(SystemHelper.appDataDir)
    })

    DirSpaceUsed {
        id: dirSpaceUsed
        onFinished: function(reqid, bytes) {
            if (reqid === cacheReqId) cacheSize = bytes
            else if (reqid === configReqId) configSize = bytes
            else if (reqid === dataReqId) dataSize = bytes
        }
    }

    component FolderTemplate: GroupBox {
        id: groupBox
        Layout.preferredWidth: control.availableWidth - 20
        Layout.margins: 10
        Material.background: MaterialSet.theme[Material.theme]["highlight"]
        Material.elevation: MaterialSet.theme[Material.theme]["elevation"]

        required property string folder
        required property string text
        required property real bytes
        signal wipe()

        ColumnLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: appTipSize
                text: groupBox.text
                elide: Text.ElideRight
            }
            RowLayout {
                Image {
                    Layout.preferredHeight: appRowHeight
                    Layout.preferredWidth: appRowHeight
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/image-open-folder"
                }
                Label {
                    Layout.fillWidth: true
                    text: groupBox.folder
                    elide: Text.ElideLeft
                }
            }
            RowLayout {
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    font.italic: true
                    text: qsTr("Used: %1").arg(prettyBytes(groupBox.bytes))
                }
                DelayButton {
                    enabled: groupBox.bytes
                    delay: appTipTimeout
                    text: qsTr("Clear folder")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Wipe %1 files").arg(groupBox.title.toLowerCase())
                    ToolTip.delay: appTipDelay
                    ToolTip.timeout: appTipTimeout
                    onActivated: groupBox.wipe()
                }
            }
        }
    }

    MyFlickable {
        anchors.fill: parent
        contentWidth: columnLayout.width
        contentHeight: columnLayout.height

        ColumnLayout {
            id: columnLayout

            RowLayout {
                Layout.preferredWidth: control.availableWidth - 20
                Layout.margins: 10
                ImageButton {
                    source: "qrc:/image-wipe-data"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://github/cbsd/cbsd")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Long press the button to clear folder<br><b>ATTENTION</b>: The <i>App's SSH key</i> will be lost after clearing the Config!")
                }
            }

            FolderTemplate {
                title: qsTr("Cache")
                folder: SystemHelper.appCacheDir
                text: qsTr("Temporary files to speed up the App")
                bytes: cacheSize
                onWipe: {
                    if (SystemHelper.removeDir(folder, true)) {
                        cacheReqId = dirSpaceUsed.start(folder)
                        if (!cacheReqId) cacheSize = 0.0
                    }
                }
            }
            FolderTemplate {
                title: qsTr("Config")
                folder: SystemHelper.appConfigDir
                text: qsTr("App GUI config, general settings, SSH keys")
                bytes: configSize
                onWipe: {
                    if (SystemHelper.removeDir(folder, true)) {
                        configReqId = dirSpaceUsed.start(folder)
                        if (!configReqId) configSize = 0.0
                    }
                }
            }
            FolderTemplate {
                title: qsTr("Data")
                folder: SystemHelper.appDataDir
                text: qsTr("A set of virtual machines and shared data")
                bytes: dataSize
                onWipe: {
                    if (SystemHelper.removeDir(folder, true)) {
                        dataReqId = dirSpaceUsed.start(folder)
                        if (!dataReqId) dataSize = 0.0
                    }
                }
            }
        }
        ScrollIndicator.vertical: ScrollIndicator { }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: dirSpaceUsed.running
    }

    footer: Pane {
        RowLayout {
            anchors.fill: parent
            TintedImage {
                source: "qrc:/icon-storage"
            }
            Label {
                Layout.fillWidth: true
                text: dirSpaceUsed.storage
                elide: Text.ElideLeft
            }
            Label {
                text: qsTr("Free <i>%1</i> of <i>%2</i> total")
                    .arg(prettyMBytes(dirSpaceUsed.availMb)).arg(prettyMBytes(dirSpaceUsed.totalMb))
            }
        }
    }
}
