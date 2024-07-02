import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

FocusScope {
    id: control

    // this variables is used in the main.qml!
    readonly property string title: VMConfigSet.isValid ? VMConfigSet.valueAt("name") : qsTr("No virtual machines")
    readonly property int preferredWidth: Math.max(buttonToolStrip.width, buttonToolStrip.height)
    readonly property int textPadding: Math.max(Math.ceil(fontMetrics.font.pixelSize * 0.1), 2)
    readonly property int rowHeight: Math.ceil(fontMetrics.height + control.textPadding * 2)
    readonly property int xVelocity: 700 // horizontal pixels in seconds
    readonly property int yVelocity: 900 // vertical pixels in seconds
    readonly property int pageVelocity: 2000 // vertical pixels in seconds

    function appendText(text) {
        loggerListModel.update(text.split('\n').filter(Boolean))
    }

    readonly property list<Action> actionModel: [
        Action {
            icon.source: "qrc:/image-add"
            text: qsTr("Settings")
            onTriggered: {
                appPage("VMSettingsPage.qml")
            }
        },
        Action {
            enabled: VMConfigSet.isCreated && !VMConfigSet.isPowerOn
            icon.source: "qrc:/image-start"
            text: qsTr("Start")
            onTriggered: {
                VMConfigSet.startCurrent()
            }
        },
        Action {
            enabled: VMConfigSet.isSshUser
            icon.source: "qrc:/image-terminal"
            text: qsTr("Terminal")
            onTriggered: {
                var server = VMConfigSet.valueAt("server")
                var key = ""
                var check = false
                if (VMConfigSet.isCbsdExecutable(server)) {
                    key = SystemHelper.userHome(VMConfigSet.cbsdName) + "/.ssh/id_rsa"
                } else {
                    var conf = SystemHelper.loadObject(server + '/' + RestApiSet.authConfFile)
                    if (!conf[RestApiSet.sshPrivName]) conf = SystemHelper.loadObject(RestApiSet.authConfFile)
                    key = conf[RestApiSet.sshPrivName]
                    check = SystemHelper.loadSettings("checkServerKey", false)
                }
                if (SystemHelper.isSshPrivateKey(key)) {
                    appPage("VMTerminalPage.qml", { sshKey: key, sshCheck: check })
                } else {
                    appInfo(qsTr("No valid SSH key, will try ~/.ssh")).accepted.connect(function() {
                        appPage("VMTerminalPage.qml")
                    })
                }
            }
        },
        Action {
            enabled: VMConfigSet.isRdpHost || VMConfigSet.isVncHost
            icon.source: "qrc:/image-desktop"
            text: qsTr("Desktop")
            onTriggered: {
                appPage("VMDesktopPage.qml")
            }
        },
        Action {
            enabled: VMConfigSet.isPowerOn
            icon.source: "qrc:/image-stop"
            text: qsTr("Stop")
            onTriggered: {
                VMConfigSet.stopCurrent()
            }
        }
    ]

    FontMetrics {
        id: fontMetrics
        font: appWindow.font
    }

    ListModel {
        id: loggerListModel
        function update(list) {
            var max = 0
            if (list.length > 500) remove(0, list.length)
            for (var line of list) {
                var w = Math.ceil(fontMetrics.advanceWidth(line) + control.textPadding * 2)
                if (w > max) max = w
                append({ "text": line })
            }
            if (count) {
                if (max > listView.contentWidth) listView.contentWidth = max
                listView.contentHeight = count * control.rowHeight
                listView.positionViewAtEnd()
            } else {
                listView.contentWidth = 0
                listView.contentHeight = 0
                listView.positionViewAtBeginning()
            }
        }
    }

    Label {
        visible: !listView.visible
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: appTitleSize
        wrapMode: Text.Wrap
        text: qsTr("<i>Welcome to %1!</i><br><br>Click the button above to create your first virtual machine")
                    .arg(Qt.application.displayName)
    }
    ListView {
        id: listView
        visible: count > 0 || VMConfigSet.isValid
        focus: true
        clip: true

        model: loggerListModel
        delegate: Text {
            height: control.rowHeight
            padding: control.textPadding
            font: fontMetrics.font
            text: modelData
            color: modelData.match(/^\d\d:\d\d:\d\d\s/) ? Material.accent : Material.foreground
            wrapMode: Text.NoWrap
            verticalAlignment: Text.AlignVCenter
            //textFormat: Text.MarkdownText
        }
        state: "ButtonsTop"
        states: [
            State {
                name: "ButtonsTop"
                AnchorChanges { target: listView; anchors { left: control.left; right: control.right; top: buttonToolStrip.bottom; bottom: control.bottom }}
            }, State {
                name: "ButtonsBottom"
                AnchorChanges { target: listView; anchors { left: control.left; right: control.right; top: control.top; bottom: buttonToolStrip.top }}
            }, State {
                name: "ButtonsLeft"
                AnchorChanges { target: listView; anchors { left: buttonToolStrip.right; right: control.right; top: control.top; bottom: control.bottom }}
            }, State {
                name: "ButtonsRight"
                AnchorChanges { target: listView; anchors { left: control.left; right: buttonToolStrip.left; top: control.top; bottom: control.bottom }}
            }, State {
                name: "Dragging"
                AnchorChanges { target: listView; anchors { left: control.left; right: control.right; top: control.top; bottom: control.bottom }}
            }
        ]
        transitions: Transition { AnchorAnimation {} }

        keyNavigationEnabled: false
        flickableDirection: Flickable.AutoFlickIfNeeded
        boundsBehavior: Flickable.OvershootBounds
        transform: [
            Scale {
                origin.x: listView.width / 2
                origin.y: listView.verticalOvershoot < 0 ? listView.height : 0
                yScale: 1.0 - Math.abs(listView.verticalOvershoot / 10) / listView.height
            },
            Rotation {
                axis { x: 0; y: 1; z: 0 }
                origin.x: listView.width / 2
                origin.y: listView.height / 2
                angle: -Math.min(10, Math.max(-10, listView.horizontalOvershoot / 10))
            }
        ]
        Keys.onPressed: function(event) {
            if (!listView.count) {
                event.accepted = false
                return
            }
            switch (event.key) {
            case Qt.Key_Up:
                if ((event.modifiers & Qt.ControlModifier) && contentY > 0)
                    contentY = 0
                else flick(0, (event.modifiers & Qt.ShiftModifier) ? control.yVelocity * 2 : control.yVelocity)
                break
            case Qt.Key_Down:
                if ((event.modifiers & Qt.ControlModifier) && contentHeight > height)
                    contentY = contentHeight - height
                flick(0, (event.modifiers & Qt.ShiftModifier) ? -control.yVelocity * 2 : -control.yVelocity)
                break
            case Qt.Key_Left:
                if ((event.modifiers & Qt.ControlModifier) && contentX > 0)
                    contentX = 0
                else flick((event.modifiers & Qt.ShiftModifier) ? control.xVelocity * 2 : control.xVelocity, 0)
                break
            case Qt.Key_Right:
                if ((event.modifiers & Qt.ControlModifier) && contentWidth > width)
                    contentX = contentWidth - width
                else flick((event.modifiers & Qt.ShiftModifier) ? -control.xVelocity * 2 : -control.xVelocity, 0)
                break
            case Qt.Key_PageUp:
                flick(0, (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) ?
                          control.pageVelocity * 2 : control.pageVelocity)
                break
            case Qt.Key_PageDown:
                flick(0, (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) ?
                          -control.pageVelocity * 2 : -control.pageVelocity)
                break
            case Qt.Key_Home:
                contentX = 0
                contentY = 0
                break
            case Qt.Key_End:
                contentX = 0
                if (contentHeight > height) contentY = contentHeight - height
                break
            default:
                event.accepted = false
                return
            }
            event.accepted = true
        }
        ScrollBar.horizontal: ScrollBar { z: 2; active: listView.count }
        ScrollBar.vertical: ScrollBar { z: 3; active: listView.count }
    }

    ButtonToolStrip {
        id: buttonToolStrip
        enabled: !VMConfigSet.isBusy
        orientation: control.width > control.height ? ListView.Vertical : ListView.Horizontal
        x: orientation === ListView.Vertical ? rightEdge : 0
        readonly property int rightEdge: control.width - width
        readonly property int bottomEdge: control.height - height

        model: control.actionModel

        DragHandler {
            id: dragHandler
            xAxis.enabled: buttonToolStrip.orientation === ListView.Vertical
            yAxis.enabled: buttonToolStrip.orientation === ListView.Horizontal
            onActiveChanged: {
                if (!active) {
                    var cp, tp
                    if (xAxis.enabled) {
                        cp = control.width / 2
                        tp = target.x + target.width / 2
                        if (tp >= cp && target.x !== buttonToolStrip.rightEdge) {
                            stickyAnimationX.to = buttonToolStrip.rightEdge
                            stickyAnimationX.start()
                        } else if (tp < cp && target.x !== 0) {
                            stickyAnimationX.to = 0
                            stickyAnimationX.start()
                        }
                    } else if (yAxis.enabled) {
                        cp = control.height / 2
                        tp = target.y + target.height / 2
                        if (tp >= cp && target.y !== buttonToolStrip.bottomEdge) {
                            stickyAnimationY.to = buttonToolStrip.bottomEdge
                            stickyAnimationY.start()
                        } else if (tp < cp && target.y !== 0) {
                            stickyAnimationY.to = 0
                            stickyAnimationY.start()
                        }
                    }
                } else listView.state = "Dragging"
            }
        }
        NumberAnimation on x {
            id: stickyAnimationX
            running: false; easing.type: Easing.OutQuad
            onFinished: listView.state = buttonToolStrip.x ? "ButtonsRight" : "ButtonsLeft"
        }
        NumberAnimation on y {
            id: stickyAnimationY
            running: false; easing.type: Easing.OutQuad
            onFinished: listView.state = buttonToolStrip.y ? "ButtonsBottom" : "ButtonsTop"
        }
        onOrientationChanged: Qt.callLater(function() {
            if (orientation === ListView.Horizontal)    listView.state = buttonToolStrip.y ? "ButtonsBottom" : "ButtonsTop"
            else if (orientation === ListView.Vertical) listView.state = buttonToolStrip.x ? "ButtonsRight" : "ButtonsLeft"
        })
    }
}
