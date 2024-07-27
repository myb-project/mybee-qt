import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Page {
    id: control
    padding: 10
    title: qsTr("About")

    header: Item {
        AnimatedImage {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/image-cbsd-movie"
            //onCurrentFrameChanged: print(currentFrame)
            paused: currentFrame >= 15
        }
    }

    Label {
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: appTitleSize
        wrapMode: Text.Wrap
        text: qsTr("<p><b>%1 v%2</b><br>for %3 (%4 %5)</p>
<p>A simple tool for managing virtual machines on cloud servers based on the CBSD system</p><br>
<p><i>Copyright (c) 2024 Oleg Ginzburg, olevole (a) yandex.ru<br>
Copyright (c) 2024 Vladimir Vorobyev, b800xy (a) yandex.ru</i></p>")
                .arg(Qt.application.displayName)
                .arg(Qt.application.version)
                .arg(SystemHelper.platformOS)
                .arg(SystemHelper.kernelType)
                .arg(SystemHelper.kernelVersion)
    }

    footer: Pane {
        ColumnLayout {
            anchors.fill: parent
            Label {
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.Wrap
                onLinkHovered: function(link) { SystemHelper.setCursorShape(link ? Qt.PointingHandCursor : -1) }
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                text: qsTr("Running on <a href='http://www.qt.io'>Qt %1</a> for %2")
                        .arg(qtVersion)
                        .arg(SystemHelper.buildAbi)
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.Wrap
                onLinkHovered: function(link) { SystemHelper.setCursorShape(link ? Qt.PointingHandCursor : -1) }
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                text: qsTr("<a href='https://opensource.org/licenses/mit-license.php'>MIT License</a>")
            }
        }
    }
}
