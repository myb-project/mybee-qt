import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: control
    modal: true
    focus: true
    anchors.centerIn: parent //Overlay.overlay
    width: Math.max(appWindow.minimumWidth, appWindow.width / 3)
    padding: 20

    Material.background: MaterialSet.theme[Material.theme]["highlight"]
    Material.elevation: MaterialSet.theme[Material.theme]["elevation"]

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
    }

    header: ColumnLayout {
        RowLayout {
            TitleLabel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                verticalAlignment: Qt.AlignBottom
                text: control.title
            }
            ToolButton {
                enabled: control.closePolicy !== Popup.NoAutoClose
                focusPolicy: Qt.NoFocus
                icon.source: "qrc:/icon-close"
                onClicked: control.close()
            }
        }
        MenuSeparator {
            Layout.fillWidth: true
        }
    }
}
