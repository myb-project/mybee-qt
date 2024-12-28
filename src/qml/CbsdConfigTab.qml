import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

DropDownView {
    id: control
    enabled: VMConfigSet.cbsdPath

    header: headerComponent
    model: control.dropDownModel
    myClassName: control.toString().match(/.+?(?=_)/)[0]

    property bool modified: false

    Component.onCompleted: {
        setIndexEnable(0, true, false)
        setIndexEnable(1, true)
    }

    readonly property var dropDownModel: [
        {   // index 0
            icon: "qrc:/icon-database-setup",
            text: qsTr("Main executable"),
            list: [ { text: QT_TR_NOOP("Local install"), item: cbsdLocalComponent },
                    { text: QT_TR_NOOP("SSH execute"), item: cbsdSshComponent } ]
        },{   // index 1
            icon: "qrc:/icon-database-query",
            text: qsTr("Control commands"),
            list: [ { text: QT_TR_NOOP("Get capability"), item: cbsdCapabilitiesComponent },
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
            width: control.width
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-drive"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://github.com/myb-project/mybee-qt")
                    Image {
                        anchors.bottom: parent.bottom
                        source: "qrc:/image-overlay-gear"
                        sourceSize: Qt.size(parent.width / 2, parent.height / 2)
                        RotationAnimator on rotation {
                            running: control.enabled
                            loops: Animation.Infinite
                            direction: RotationAnimator.Clockwise
                            from: 0; to: 360; duration: 4000
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Change the <i>%1 System</i> settings<br><b>ATTENTION</b>: Be careful what you do!").arg(VMConfigSet.cbsdName.toUpperCase())
                }
            }
        }
    }

    // Main executable

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

    // Control commands

    property string cbsdCapabilities: VMConfigSet.cbsdCapabilities
    Component {
        id: cbsdCapabilitiesComponent
        MyTextField {
            text: VMConfigSet.cbsdCapabilities
            onEditingFinished: {
                control.cbsdCapabilities = text
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
}
