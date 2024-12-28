import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("Cloud Server")
    enabled: !VMConfigSet.isBusy

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    readonly property var urlSchemeModel: [ "http", "https", "ssh", "file" ] // must be lower case!
    readonly property var urlPortModel:   [ 80, 443, 22, 0 ]
    property int currentScheme: 0
    property string lastServers
    property string currentServer
    property string currentSshKey

    readonly property var dropDownModel: [
        {   // index 0
            icon: "qrc:/icon-cloud-server",
            text: QT_TR_NOOP("%1 connection").arg(VMConfigSet.cbsdName.toUpperCase()),
            list: [ { item: schemeComponent },
                    { text: QT_TR_NOOP("History"), item: historyComponent },
                    { text: QT_TR_NOOP("Address"), item: addressComponent },
                    { text: QT_TR_NOOP("Path"), item: pathComponent } ]
        },{ // index 1
            icon: "qrc:/icon-ssh-key",
            text: QT_TR_NOOP("SSH key"),
            list: [ { item: sshKeyComponent } ]
        }
    ]

    Component.onCompleted: {
        lastServers = SystemHelper.loadSettings(myClassName + "/lastServers", "")
        if (!lastServers) {
            var env = SystemHelper.envVariable("CLOUD_URL")
            if (Url.isValidAt(env)) lastServers = env
        }
        if (VMConfigSet.cbsdPath && !lastServers.includes(VMConfigSet.cbsdPath)) {
            if (lastServers) lastServers += ' '
            lastServers += "file:" + VMConfigSet.cbsdPath
        }
        if (lastServers) {
            var list = lastServers.split(' ').filter(Boolean)
            for (var i = list.length - 1; i >= 0; i--) {
                urlInModel.location = list[i].trim()
                if (urlInModel.valid && ~urlSchemeModel.indexOf(urlInModel.scheme) && VMConfigSet.isSchemeEnabled(urlInModel.scheme))
                    historyListModel.insert(0, { "text": urlInModel.text })
            }
        }
        var idx = control.urlSchemeModel.indexOf(urlInModel.scheme)
        currentScheme = ~idx ? idx : 0

        appDelay(appTinyDelay, function() {
            dropDownView.setIndexEnable(0, true)
            dropDownView.setIndexEnable(1, true)
            urlOutModel.location = urlInModel.location
        })
    }

    Component.onDestruction: {
        if (currentServer) {
            var list = lastServers.split(' ').filter(Boolean)
            var index = list.indexOf(currentServer)
            if (index) {
                if (~index) list.splice(index, 1)
                else if (list.length >= 10) list.splice(9, list.length - 9)
                list.splice(0, 0, currentServer)
                SystemHelper.saveSettings(myClassName + "/lastServers", list.join(' '))
            }
        }
    }

    function setCurrentServer() {
        if (!urlOutModel.valid || !currentSshKey) return
        if (urlOutModel.local && !SystemHelper.isExecutable(urlOutModel.path)) {
            appWarning(qsTr("%1: Not an executable").arg(urlOutModel.path))
            return
        }
        if (!SystemHelper.isSshPrivateKey(currentSshKey)) {
            appWarning(qsTr("%1: Not an ssh key").arg(currentSshKey))
            return
        }
        if (urlOutModel.port) {
            for (var i = 0; i < urlPortModel.length; i++) {
                if (urlOutModel.port === urlPortModel[i] && urlOutModel.scheme === urlSchemeModel[i]) {
                    urlOutModel.port = 0
                    break
                }
            }
        }
        if (urlOutModel.scheme === "ssh" && !urlOutModel.userName)
            urlOutModel.userName = "root"

        var folder = SystemHelper.fileName(urlOutModel.text)
        if (!SystemHelper.saveObject(folder + "/lastServer", { "server": urlOutModel.text, "ssh_key": currentSshKey })) {
            appError(qsTr("Can't save current configuration"))
            return
        }
        currentServer = urlOutModel.text
        appPage("VMProfilePage.qml", { "currentFolder": folder })
    }

    UrlModel {
        id: urlInModel
        scheme: urlSchemeModel[currentScheme]
    }

    UrlModel {
        id: urlOutModel
        scheme: urlSchemeModel[currentScheme]
    }

    ListModel {
        id: historyListModel
    }

    TextMetrics {
        id: portTextMetrics
        font: control.font
        text: "65535"
    }

    Component {
        id: headerComponent
        Pane {
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: urlOutModel.remote ? "qrc:/image-drive-network" : (urlOutModel.local ? "qrc:/image-drive" : "qrc:/image-drive-offline")
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://github.com/myb-project/mybee-qt")
                    Image {
                        anchors.bottom: parent.bottom
                        visible: urlOutModel.valid
                        source: "qrc:/image-overlay-database"
                        sourceSize: Qt.size(parent.width / 2, parent.height / 2)
                    }
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Create a virtual machine using the appropriate <i>%1 cloud server</i> and <i>SSH access key</i>").arg(VMConfigSet.cbsdName.toUpperCase())
                }
            }
        }
    }

    // Server connection

    Component {
        id: schemeComponent
        RowLayout {
            Repeater {
                model: control.urlSchemeModel
                RadioButton {
                    focusPolicy: Qt.NoFocus
                    enabled: VMConfigSet.isSchemeEnabled(modelData)
                    checked: index === control.currentScheme
                    text: modelData.toUpperCase()
                    onToggled: {
                        if (!checked) return
                        urlInModel.clear()
                        urlOutModel.clear()
                        control.currentScheme = index
                        for (var i = 0; i < historyListModel.count; i++) {
                            var txt = historyListModel.get(i).text
                            if (txt.startsWith(modelData + ':')) {
                                urlInModel.location = txt
                                urlOutModel.location = txt
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: historyComponent
        ComboBox {
            enabled: count
            model: historyListModel
            onActivated: function(index) {
                var idx = control.urlSchemeModel.indexOf(Url.schemeAt(currentText))
                if (~idx) {
                    control.currentScheme = idx
                    urlInModel.location = currentText
                    urlOutModel.location = currentText
                } else urlOutModel.clear()
            }
        }
    }

    Component {
        id: addressComponent
        RowLayout {
            MyTextField {
                id: hostTextField
                Layout.fillWidth: true
                enabled: control.urlSchemeModel[control.currentScheme] !== "file"
                placeholderText: urlInModel.scheme === "ssh" ? qsTr("User@Host") : qsTr("Host")
                validator: RegularExpressionValidator { regularExpression: /^[^<>:;,?"*|\\ /]+$/ }
                text: (urlInModel.scheme === "ssh" && urlInModel.userName) ?
                          (urlInModel.userName + '@' + urlInModel.host) : urlInModel.host
                onTextEdited: {
                    if (urlInModel.scheme === "ssh") {
                        var pair = text.split('@')
                        if (pair.length === 2) {
                            urlOutModel.userName = pair[0]
                            urlOutModel.host = pair[1]
                            return
                        }
                    }
                    urlOutModel.host = text
                }
                FlashingPoint { visible: enabled && !hostTextField.text }
            }
            MyTextField {
                property int portIndex: control.urlSchemeModel.indexOf(urlInModel.scheme)
                property int portNumber: urlInModel.port ? urlInModel.port : (~portIndex ? control.urlPortModel[portIndex] : 0)
                Layout.preferredWidth: portTextMetrics.advanceWidth + leftPadding + rightPadding
                enabled: hostTextField.enabled
                placeholderText: qsTr("Port")
                validator: IntValidator { bottom: 0; top: 65535 }
                text: portNumber ? portNumber : ""
                onTextEdited: urlOutModel.port = parseInt(text)
            }
        }
    }

    Component {
        id: pathComponent
        RowLayout {
            MyTextField {
                id: pathTextField
                Layout.fillWidth: true
                enabled: control.urlSchemeModel[control.currentScheme] === "file"
                placeholderText: qsTr("Executable")
                validator: RegularExpressionValidator { regularExpression: /^[^<>:;,?"*|\\ ]+$/ }
                text: urlInModel.path
                onTextEdited: urlOutModel.path = text
                FlashingPoint { visible: enabled && !pathTextField.text }
            }
            SquareButton {
                enabled: pathTextField.enabled
                icon.source: "qrc:/icon-open-folder"
                ToolTip.text: qsTr("Select file")
                onClicked: {
                    var prop = { "title": qsTr("Select %1 executable").arg(VMConfigSet.cbsdName.toUpperCase()) }
                    if (pathTextField.text) {
                        var dir = SystemHelper.pathName(pathTextField.text)
                        if (dir) prop["path"] = dir
                    }
                    var dlg = Qt.createComponent("MyFileDialog.qml").createObject(control, prop)
                    if (!dlg) { appToast(qsTr("Can't load MyFileDialog.qml")); return }
                    dlg.accepted.connect(function() {
                        var path = dlg.filePath()
                        if (SystemHelper.isExecutable(path)) pathTextField.text = path
                        else appInfo(qsTr("The file is not executable"))
                    })
                    dlg.open()
                }
            }
        }
    }

    // Ssh key

    Component {
        id: sshKeyComponent
        AppSshKeyPane {
            currentServer: urlInModel.text
            onSshKeyFileChanged: {
                if (!sshKeyFile || SystemHelper.isSshPrivateKey(sshKeyFile))
                    control.currentSshKey = sshKeyFile
                else appWarning(qsTr("%1: Not an ssh key").arg(sshKeyFile))
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
        show: urlOutModel.valid
        GridLayout {
            anchors.fill: parent
            flow: appPortraitView ? GridLayout.TopToBottom : GridLayout.LeftToRight
            rows: 2
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                font.pointSize: appTitleSize
                text: urlOutModel.text
                elide: Text.ElideRight
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                font.pointSize: appTitleSize
                text: control.currentSshKey ? control.currentSshKey : qsTr("Please select ssh key")
                elide: Text.ElideLeft
            }
            SquareButton {
                Layout.rowSpan: appPortraitView ? 2 : 1
                enabled: control.currentSshKey
                highlighted: true
                text: qsTr("Profile")
                icon.source: "qrc:/icon-page-next"
                ToolTip.text: qsTr("Get VM profiles")
                onClicked: setCurrentServer()
            }
        }
    }
}
