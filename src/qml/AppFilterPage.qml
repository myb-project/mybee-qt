import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Page {
    id: control
    enabled: VMConfigSet.configCount
    title: qsTr("Properties Filter")

    property var matchedKeys: []
    property var matchedText: []

    Component.onCompleted: {
        var names = [], list = appMainPage.filterText.split("; ")
        for (var i = 0; i < list.length; i++) {
            var tok = list[i].split('=')
            if (tok.length > 1) names.push(tok[0])
        }
        var map = VMConfigSet.currentConfig
        for (var key in map) {
            filterListModel.append({
                "varName": key,
                "varValue": map[key].toString(),
                "varCheck": names.includes(key)
            })
        }
        appDelay(appTinyDelay, ensureRowVisible)
    }

    function ensureRowVisible() {
        for (var i = 0; i < rowRepeater.count; i++) {
            var row = rowRepeater.itemAt(i)
            if (row.checked) {
                if (row.y > myFlickable.height - headerLayout.height - row.height * 1.5)
                    myFlickable.contentY = row.y - myFlickable.height / 2
                return
            }
        }
    }

    function rematch(name, value) {
        var keys = [], text = []
        var map = VMConfigSet.configMap
        for (var key in map) {
            var obj = map[key]
            if (obj.hasOwnProperty(name) && obj[name].toString() === value) {
                keys.push(key)
                var txt = name + '=' + value
                if (!text.includes(txt)) text.push(txt)
            }
        }
        matchedKeys = keys
        matchedText = text
    }

    ListModel {
        id: filterListModel
    }

    ButtonGroup {
        id: radioGroup
    }

    MyFlickable {
        id: myFlickable
        anchors.fill: parent
        focus: true
        contentWidth: columnLayout.width
        contentHeight: columnLayout.height
        Behavior on contentY {
            SequentialAnimation {
                SmoothedAnimation { velocity: 1500 }
                ScriptAction { script: myFlickable.returnToBounds() }
            }
        }

        ColumnLayout {
            id: columnLayout

            RowLayout {
                id: headerLayout
                Layout.preferredWidth: control.availableWidth - 20
                Layout.margins: 10
                ImageButton {
                    source: "qrc:/image-filter"
                    text: qsTr("Visit the project homepage")
                    onClicked: Qt.openUrlExternally("https://github.com/myb-project/mybee-qt")
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Set a filter to display matching objects")
                }
            }

            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                flow: GridLayout.TopToBottom
                columns: Math.floor(control.availableWidth / 350)
                rows: Math.ceil(filterListModel.count / columns)

                Repeater {
                    id: rowRepeater
                    model: filterListModel
                    RowLayout {
                        spacing: 0
                        property alias checked: radioButton.checked
                        RadioButton {
                            id: radioButton
                            Layout.preferredWidth: 140
                            checked: varCheck
                            text: varName
                            ButtonGroup.group: radioGroup
                            onToggled: {
                                if (checked) rematch(text, textField.text)
                            }
                        }
                        MyTextField {
                            id: textField
                            Layout.preferredWidth: 200
                            placeholderText: varValue
                            text: varValue
                            onEditingFinished: {
                                if (radioButton.checked) rematch(radioButton.text, text)
                            }
                        }
                    }
                }
            }
        }

        ScrollBar.vertical: ScrollBar {}
    }

    footer: DropDownPane {
        show: control.matchedKeys.length
        onVisibleChanged: ensureRowVisible()
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: appTitleSize
                text: qsTr("%1 [ %2 / %3 ]").arg(control.matchedText).arg(control.matchedKeys.length).arg(VMConfigSet.configCount)
                elide: Text.ElideRight
            }
            SquareButton {
                highlighted: true
                text: qsTr("Filter")
                icon.source: "qrc:/icon-filter-plus"
                ToolTip.text: qsTr("Set filter")
                onClicked: {
                    appMainPage.setFilter(control.matchedKeys)
                    appMainPage.filterText = control.matchedText.join("; ")
                    appStackView.pop()
                }
            }
        }
    }
}
