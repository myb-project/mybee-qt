import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

MyDialog {
    id: control
    title: qsTr("Please wait")
    standardButtons: Dialog.Abort
    closePolicy: Popup.NoAutoClose

    property string currentState: qsTr("Work in progress...")
    property string mirrorNumber: ""
    property string loadFile: ""
    property string loadSize: ""
    property string totalSize: ""

    UrlModel {
        id: urlModel
    }

    Connections {
        target: SshProcess
        function onFinished(code) { control.close() }
        function onStdOutputChanged(text) { parseStdOutput(text) }
        function onStdErrorChanged(text) { parseStdError(text) }
    }

    Connections {
        target: SystemProcess
        function onFinished(code) { control.close() }
        function onStdOutputChanged(text) { parseStdOutput(text) }
        function onStdErrorChanged(text) { parseStdError(text) }
    }

    function parseStdOutput(text) {
        //console.debug("1=", text)
        if (text.startsWith("vm_iso_path:")) {
            var iso = text.slice(12).trim()
            if (iso !== "changed") {
                control.currentState = qsTr("Find %1...").arg(iso)
            }
        } else if (text.startsWith("Processing:")) {
            urlModel.location = text.slice(11).trim()
            if (urlModel.remote) {
                control.currentState = urlModel.host
                control.loadFile = urlModel.fileName()
            }
        } else if (text.endsWith("Passed\n")) {
            control.currentState = qsTr("Extracting image...")
            control.loadSize = ""
        } else if (text.includes("iso as:")) {
            control.loadFile = text.slice(text.indexOf("iso as:") + 7).trim()
        } else if (text.includes("image init")) {
            control.currentState = qsTr("Image initialization...")
            progressBar.indeterminate = true
        } else if (text.startsWith("Boot device:")) {
            control.currentState = qsTr("Booting image...")
            progressBar.value = 100
            progressBar.indeterminate = false
        }
    }

    function parseStdError(text) {
        //console.debug("2=", text)
        var list, val
        if (text.startsWith(" * [ ")) {
            list = text.slice(5).trim().split(' ').filter(Boolean)
            if (list.length >= 3) {
                urlModel.location = list[2]
                control.mirrorNumber = urlModel.remote ? list[0] : ""
            }
        } else if (text.startsWith('\r')) {
            list = text.trim().split(' ').filter(Boolean)
            if (list.length < 4) return
            val = parseInt(list[2])
            if (!isNaN(val) && val >= 0 && val <= 100) {
                control.totalSize = list[1]
                control.loadSize = list[3]
                progressBar.value = val
                progressBar.indeterminate = false
            }
        } else if (text.startsWith("...")) {
            val = parseInt(text.slice(3))
            if (!isNaN(val) && val >= 0 && val <= 100) {
                progressBar.value = val
                progressBar.indeterminate = false
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: control.padding

        RowLayout {
            Layout.fillWidth: true
            AnimatedImage {
                Layout.preferredWidth: appIconSize
                Layout.preferredHeight: appIconSize
                source: "qrc:/image-progress"
                paused: !progressBar.indeterminate
            }
            Label {
                Layout.fillWidth: true
                font.pointSize: appTitleSize
                text: control.currentState
                //wrapMode: Text.Wrap
                elide: Text.ElideRight
            }
        }

        StackLayout {
            currentIndex: control.totalSize ? 1 : 0
            RowLayout {
                Label { text: control.mirrorNumber }
                Item { Layout.fillWidth: true }
                Label { text: urlModel.host }
            }
            RowLayout {
                Label { text: control.loadFile }
                Item { Layout.fillWidth: true }
                Label { text: control.loadSize ? (control.loadSize + " / " + control.totalSize) : "" }
            }
        }

        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            from: 0; to: 100
            indeterminate: true
        }
    }

    onRejected: {
        if (SshProcess.running) SshProcess.cancel()
        if (SystemProcess.running) SystemProcess.cancel()
    }
}
