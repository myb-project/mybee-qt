import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Page {
    id: control
    title: qsTr("About")

    readonly property string donateLink: "https://www.patreon.com/clonos"
    readonly property var consistsOf: [
        QT_TR_NOOP("Running on"), QT_TR_NOOP("<a href='https://www.qt.io'>Qt %1</a>").arg(SystemHelper.qtRCVersion),
        QT_TR_NOOP("Using SSL"),  QT_TR_NOOP("<a href='https://openssl-library.org'>%1</a>").arg(SystemHelper.sslVersion),
        QT_TR_NOOP("With SSH"),   QT_TR_NOOP("<a href='https://www.libssh.org'>LibSSH %1</a>").arg(SystemHelper.sshVersion),
        QT_TR_NOOP("With RDP"),   QT_TR_NOOP("<a href='https://www.freerdp.com'>FreeRDP %1</a>").arg(SystemHelper.rdpVersion),
        QT_TR_NOOP("With VNC"),   QT_TR_NOOP("<a href='https://libvnc.github.io'>LibVNCClient %1</a>").arg(SystemHelper.vncVersion),
        QT_TR_NOOP("Distributed"),QT_TR_NOOP("<a href='https://opensource.org/licenses/mit-license.php'>under MIT License</a>")
    ]

    MyFlickable {
        anchors.fill: parent
        contentWidth: columnLayout.width
        contentHeight: columnLayout.height

        ColumnLayout {
            id: columnLayout
            width: control.availableWidth
            spacing: 10

            AnimatedImage {
                id: animatedImage
                Layout.maximumWidth: control.width
                Layout.alignment: Qt.AlignHCenter
                source: "qrc:/image-cbsd-movie"
                paused: currentFrame >= 15
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                font.pointSize: appExplainSize
                text: qsTr("<b>%1 v%2</b>").arg(Qt.application.displayName).arg(Qt.application.version)
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: appTitleSize
                wrapMode: Text.Wrap
                text: qsTr("for %1 (%2 %3)<br><br><i>A simple tool for managing virtual machines on cloud servers based on the CBSD system</i>").arg(SystemHelper.platformOS).arg(SystemHelper.kernelType).arg(SystemHelper.kernelVersion)
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                textFormat: Text.PlainText
                text: qsTr("\u00a9 2023-2025, Oleg Ginzburg <olevole@yandex.ru>\n\u00a9 2023-2025, Vladimir Vorobyev <b800xy@yandex.ru>")
            }

            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: 2
                Repeater {
                    model: control.consistsOf
                    Label {
                        text: modelData
                        onLinkHovered: function(link) { SystemHelper.setCursorShape(link ? Qt.PointingHandCursor : -1) }
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }
                }
            }

            MenuSeparator {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: animatedImage.width
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: appTipSize
                padding: 5
                wrapMode: Text.Wrap
                text: qsTr("Become a financial contributor and help us sustain our community. All funds are returned to the project (hosting, equipment, support) and invested in the development of a number of areas.")
            }
        }
    }

    footer: DropDownPane {
        show: animatedImage.paused
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                font.pointSize: appTitleSize
                text: qsTr("Financial Contributors")
                elide: Text.ElideLeft
            }
            SquareButton {
                text: qsTr("Donate")
                icon.source: "qrc:/icon-donate"
                ToolTip.text: control.donateLink
                onClicked: Qt.openUrlExternally(control.donateLink)
            }
        }
    }
}
