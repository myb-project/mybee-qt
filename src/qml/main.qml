import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
//import QmlCustomModules 1.0

ApplicationWindow {
    id: appWindow
    width: appStartWidth
    height: appStartHeight
    minimumWidth: 360
    minimumHeight: 400
    title: Qt.application.displayName
    visible: true
    //visibility: isMobile ? ApplicationWindow.FullScreen : ApplicationWindow.Windowed

    readonly property int appStartWidth:    400 // HD: 1280
    readonly property int appStartHeight:   700 // HD: 720
    readonly property int appFitWidth:      390 //5 * (appButtonSize + appTextPadding * 2)
    readonly property int appFitHeight:     Math.max(appFitWidth * appScreenRatio, appStartHeight)
    readonly property real scrPixelDensity: (isMobile && (Screen.width / Screen.pixelDensity) > 300) ?
                                                Screen.pixelDensity * 2 : Screen.pixelDensity
    readonly property bool appScreenTiny:   (Screen.width / scrPixelDensity) < 120
    readonly property bool appScreenShort:  (Screen.height / scrPixelDensity) < 120 ||
                                                (isMobile && (Screen.height / Screen.width) < 0.6)
    readonly property bool appScreenHuge:   (Screen.width / scrPixelDensity) >= (23.5 * 25.4) // 27" monitor
    readonly property real appScreenRatio:  Screen.width > Screen.height ? Screen.width / Screen.height
                                                                         : Screen.height / Screen.width
    readonly property bool appPortraitView: appWindow.height > appWindow.width
    readonly property int  appExplainSize:  font.pointSize + 5
    readonly property int  appTitleSize:    font.pointSize + 2 //(font.pointSize >= 12 ? 2 : 1)
    readonly property int  appTipSize:      font.pointSize - 1 //(font.pointSize >= 12 ? 2 : 1)
    readonly property int  appIconSize:     (appToolButton.height * 1.2) & ~1 // make an even number
    readonly property int  appButtonSize:   (appToolButton.height * 1.4) & ~1 // make an even number
    readonly property int  appRowHeight:    Math.ceil(font.pixelSize * 2.5) & ~1 // make an even number
    readonly property int  appTextPadding:  Math.max(Math.ceil(font.pixelSize * 0.25), 2)
    readonly property int  appTinyDelay:    250 // milliseconds
    readonly property int  appTipDelay:     750 // milliseconds
    readonly property int  appTipTimeout:   1500 // milliseconds
    readonly property size appDesktopSize:  Qt.size(Screen.desktopAvailableWidth,
                                                    Screen.desktopAvailableHeight - header.height)
    property bool appShowDebug: true
    property bool appForceQuit: false
    property string lastSudoPswd

    property real appOrigFontSize: 0.0
    property int appMaterialTheme: MaterialSet.defaultTheme
    Material.theme:      appMaterialTheme
    Material.accent:     MaterialSet.theme[Material.theme]["accent"]
    Material.background: MaterialSet.theme[Material.theme]["background"]
    Material.foreground: MaterialSet.theme[Material.theme]["foreground"]
    Material.primary:    active ? MaterialSet.theme[Material.theme]["primary"]
                                : MaterialSet.theme[Material.theme]["shadePrimary"]

    // for debugging purposes only
    //onActiveFocusControlChanged: console.debug("activeFocusControl", activeFocusControl)
    //onActiveFocusItemChanged: console.debug("activeFocusItem", activeFocusItem)

    MySettings {
        id: appSettings
        property alias width: appWindow.width
        property alias height: appWindow.height
        property alias materialTheme: appWindow.appMaterialTheme
        property alias origFontSize: appWindow.appOrigFontSize
        property alias splitHFactor: appMainPage.splitHFactor
        property alias splitVFactor: appMainPage.splitVFactor
    }
    onFontChanged: appSettings.setValue("lastFontSize", appWindow.font.pointSize)

    Component.onCompleted: {
        if (appWindow.width === appStartWidth && appWindow.height === appStartHeight) {
            appWindow.width = appFitWidth
            appWindow.height = appFitHeight
        }
        if (appOrigFontSize) {
            var ps = appSettings.value("lastFontSize")
            if (ps && ps !== appWindow.font.pointSize) appWindow.font.pointSize = ps
        } else appOrigFontSize = appWindow.font.pointSize

        var env = SystemHelper.envVariable("MYBEE_QT_DEBUG")
        appShowDebug = env ? (env === "1" || env.toLowerCase() === "true")
                           : SystemHelper.loadSettings("showDebug", true)

        SystemProcess.execError.connect(appError)
        SystemProcess.stdOutputChanged.connect(cbsdOutputProgress)
        SystemProcess.stdErrorChanged.connect(function(text) {
            if (text === VMConfigSet.sudoPswd) {
                var dlg = appDialog("PasswordDialog.qml", { user: Qt.platform.os === "windows" ? "Administrator" : "root",
                                        path: VMConfigSet.cbsdPath, password: lastSudoPswd })
                dlg.rejected.connect(SystemProcess.cancel)
                dlg.accepted.connect(function() {
                    lastSudoPswd = dlg.password
                    if (!lastSudoPswd) SystemProcess.cancel()
                    else SystemProcess.stdInput(lastSudoPswd)
                })
            } else appTextLogger(text)
        })

        SshProcess.execError.connect(appWarning)
        SshProcess.stdOutputChanged.connect(cbsdOutputProgress)
        SshProcess.stdErrorChanged.connect(appTextLogger)
        SshProcess.knownHostChanged.connect(function() { sshServerDialog(SshProcess) })
        SshProcess.askQuestions.connect(function(info, prompts) { sshPasswordDialog(SshProcess, info, prompts) })

        HttpRequest.recvError.connect(appWarning)
        RestApiSet.error.connect(appWarning)
        RestApiSet.message.connect(appTimeLogger)
        VMConfigSet.error.connect(appWarning)
        VMConfigSet.message.connect(appTimeLogger)

        if (SystemHelper.userHome(VMConfigSet.cbsdName)) {
            var path = SystemHelper.findExecutable(VMConfigSet.cbsdName)
            if (path && !SystemHelper.groupMembers(VMConfigSet.cbsdName).includes(SystemHelper.userName)) {
                appError(qsTr("User <b>%1</b> must be a member of the <b>%2</b> group to use <i>%3</i>")
                         .arg(SystemHelper.userName).arg(VMConfigSet.cbsdName).arg(path))
                            .accepted.connect(function() { VMConfigSet.clusterEnabled = true })
                return
            }
        }
        VMConfigSet.clusterEnabled = true

        if (Qt.application.os === "android") {
            Qt.callLater(function() {
                if (!SystemHelper.setAndroidPermission(
                            ["android.permission.INTERNET",
                             "android.permission.WRITE_EXTERNAL_STORAGE",
                             "android.permission.ACCESS_NETWORK_STATE"]))
                    appError(qsTr("Insufficient Android permissions"))
            })
        }
    }

    onClosing: function(close) {
        if (isMobile && appStackView.depth > 1) {
            appStackView.pop(null)
            close.accepted = false
        } else if (VMConfigSet.isBusy && !appForceQuit) {
            appWarning(qsTr("The system is busy, quit anyway?"), Dialog.Yes | Dialog.No).accepted.connect(function() {
                appForceQuit = true
                Qt.callLater(Qt.quit)
            })
            close.accepted = false
        }
    }

    function appTimeLogger(text) {
        if (appShowDebug) appMainPage.appendText(Qt.formatTime(new Date(), Qt.ISODateWithMs) + ' ' + text)
    }
    function appTextLogger(text) {
        if (appShowDebug && VMConfigSet.currentProgress < 0) appMainPage.appendText(text)
    }

    function cbsdOutputProgress(text) {
        if (!VMConfigSet.currentProgress) {
            VMConfigSet.currentProgress = 1
            appDialog("VMProgressDialog.qml")
        } else appTextLogger(text)
    }

    property var appLastDialog
    function destroyLastDialog() {
        if (appLastDialog) {
            appLastDialog.destroy()
            appLastDialog = undefined
        }
    }
    function appDialog(qml, prop = {}) {
        destroyLastDialog()
        appLastDialog = Qt.createComponent(qml).createObject(appWindow, prop)
        if (appLastDialog) {
            appLastDialog.closed.connect(destroyLastDialog)
            appLastDialog.open()
        } else appToast(qsTr("Can't load %1").arg(qml))
        return appLastDialog
    }

    function appError(text, buttons = Dialog.Abort | Dialog.Ignore) {
        if (!text) text = qsTr("Unexpected error")
        var dlg = appDialog("BriefDialog.qml", { "type": BriefDialog.Type.Error, "text": text, "standardButtons": buttons })
        if (dlg && (buttons & Dialog.Abort)) dlg.rejected.connect(Qt.quit)
        return dlg
    }

    function appWarning(text, buttons = Dialog.Ignore) {
        return appDialog("BriefDialog.qml", { "type": BriefDialog.Type.Warning, "text": text, "standardButtons": buttons })
    }

    function appInfo(text, buttons = Dialog.Ok) {
        return appDialog("BriefDialog.qml", { "type": BriefDialog.Type.Info, "text": text, "standardButtons": buttons })
    }

    function sshServerDialog(sshSession) {
        var dlg_type = BriefDialog.Type.Warning, dlg_button = Dialog.Yes | Dialog.No, dlg_text
        switch (sshSession.knownHost) {
        case SshSession.KnownHostUnknown:
        case SshSession.KnownHostCreated:
            dlg_type = BriefDialog.Type.Info; dlg_button = Dialog.Ok
            dlg_text = qsTr("SSH connection to host <b>%1</b> for the first time<br><br><i>Its public key will be added to my known_hosts to validate future connections</i>")
                            .arg(SystemHelper.camelCase(sshSession.hostAddress))
            break
        case SshSession.KnownHostUpdated:
            dlg_text = qsTr("The public key of the host <b>%1</b> has been updated<br><br><i>Do you trust this host's new public key?</i>")
                            .arg(SystemHelper.camelCase(sshSession.hostAddress))
            break
        case SshSession.KnownHostUpgraded:
            dlg_text = qsTr("The public key type on host <b>%1</b> has been changed<br><br><i>Are you sure the server is valid?</i>")
                            .arg(SystemHelper.camelCase(sshSession.hostAddress))
            break
        default: return
        }
        var dlg = appDialog("BriefDialog.qml", { type: dlg_type, standardButtons: dlg_button, text: dlg_text,
                            placeholderText: qsTr("Host public key hash"), input: sshSession.pubkeyHash, readOnly: true })
        dlg.accepted.connect(sshSession.writeKnownHost)
        dlg.rejected.connect(sshSession.cancel)
    }

    function sshPasswordDialog(sshSession, info, prompts) {
        if (!prompts.length) return // should not happend
        var dlg = appDialog("SshPromptsDialog.qml", { title: qsTr("Access to %1").arg(Url.textAt(sshSession.sshUrl)),
                            info: info ? info : qsTr("Host <b>%1</b> asks the following questions").arg(SystemHelper.camelCase(sshSession.hostAddress)),
                            prompts: prompts, publicKey: SystemHelper.isSshKeyPair(sshSession.settings.privateKey) })
        dlg.accepted.connect(function() {
            sshSession.shareKey = dlg.shareKey
            sshSession.giveAnswers(dlg.answers())
        })
        dlg.rejected.connect(sshSession.cancel)
    }

    Action {
        id: appEscapeAction
        shortcut: StandardKey.Cancel
        onTriggered: {
            var request
            if (HttpRequest.running) {
                request = HttpRequest
            } else if (SshProcess.running) {
                request = SshProcess
            } else if (SystemProcess.running) {
                request = SystemProcess
            } else if (appStackView.depth > 1) {
                appStackView.pop() //appStackView.pop(null)
                return
            } else {
                appSidebarDrawer.visible = !appSidebarDrawer.visible
                return
            }
            appWarning(qsTr("The request is being processed, do you want to cancel it?"),
                       Dialog.Yes | Dialog.No).accepted.connect(request.cancel)
        }
    }
    Action {
        id: appCreateAction
        icon.source: "qrc:/icon-create"
        text: qsTr("Create")
        onTriggered: appPage("VMServerPage.qml")
    }
    Action {
        id: appStartAction
        enabled: VMConfigSet.isCreated && !VMConfigSet.isPowerOn
        icon.source: "qrc:/icon-start"
        text: qsTr("Start")
        onTriggered: VMConfigSet.startVm(VMConfigSet.currentConfig)
    }
    Action {
        id: appStopAction
        enabled: VMConfigSet.isPowerOn
        icon.source: "qrc:/icon-stop"
        text: qsTr("Stop")
        onTriggered: VMConfigSet.stopVm(VMConfigSet.currentConfig)
    }
    Action {
        id: appTerminalAction
        enabled: VMConfigSet.isSshUser
        icon.source: "qrc:/icon-terminal"
        text: qsTr("Terminal")
        onTriggered: appPage("VMTerminalPage.qml")
    }
    Action {
        id: appDesktopAction
        enabled: VMConfigSet.isRdpHost || VMConfigSet.isVncHost
        icon.source: "qrc:/icon-desktop"
        text: qsTr("Desktop")
        onTriggered: appPage("VMDesktopPage.qml")
    }
    Action {
        id: appFullScreenAction
        checkable: true
        checked: visibility === ApplicationWindow.FullScreen
        text: qsTr("Full screen")
        shortcut: "F11"
        onToggled: visibility = checked ? ApplicationWindow.FullScreen : ApplicationWindow.Windowed
    }
    Action {
        id: appCompactAction
        enabled: appWindow.visibility === ApplicationWindow.Windowed
        checkable: true
        checked: appWindow.width === appFitWidth && appWindow.height === appFitHeight
        text: qsTr("Compact size")
        shortcut: "F12"
        onToggled: {
            if (checked) {
                appWindow.width = appFitWidth
                appWindow.height = appFitHeight
            } else {
                appWindow.height = appFitHeight
                Qt.callLater(function() { appWindow.width = contentItem.height * appScreenRatio })
            }
        }
    }
    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    component ToolButtonTemplate: ToolButton {
        Layout.fillWidth: display === AbstractButton.TextUnderIcon
        Layout.fillHeight: true
        focusPolicy: Qt.NoFocus
        spacing: appTextPadding
        display: appPortraitView ? AbstractButton.TextUnderIcon : AbstractButton.TextBesideIcon
        font.pointSize: appTipSize
        font.bold: appPortraitView
    }

    header: ToolBar {
        Material.elevation: MaterialSet.theme[Material.theme]["elevation"]
        StackLayout {
            anchors.fill: parent
            currentIndex: VMConfigSet.isBusy ? 1 : 0
            RowLayout {
                spacing: 0
                ToolButton {
                    id: appToolButton
                    Layout.fillHeight: true
                    focusPolicy: Qt.NoFocus
                    action: appEscapeAction
                    icon.source: appStackView.depth > 1 ? "qrc:/icon-return" : "qrc:/icon-menu"
                    rotation: -appSidebarDrawer.position * 90
                }
                TitleLabel {
                    Layout.fillWidth: true
                    text: (appStackView.currentItem && appStackView.currentItem.title) ? appStackView.currentItem.title : VMConfigSet.valueAt("name")
                }
                ToolButtonTemplate {
                    id: appCreateButton
                    visible: appStackView.depth === 1
                    enabled: !appSidebarDrawer.visible
                    display: appPortraitView ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                    action: appCreateAction
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Creating a new virtual machine")
                    ToolTip.delay: appTipDelay
                    ToolTip.timeout: appTipTimeout
                    FlashingPoint { visible: appCreateButton.enabled && !VMConfigSet.configCount }
                }
                RowLayout {
                    visible: !appPortraitView && appStackView.depth === 1
                    enabled: !appSidebarDrawer.visible
                    spacing: 0
                    ToolButtonTemplate { action: appStartAction }
                    ToolButtonTemplate { action: appStopAction }
                    ToolButtonTemplate { action: appTerminalAction }
                    ToolButtonTemplate { action: appDesktopAction }
                }
                ToolButton {
                    Layout.fillHeight: true
                    enabled: !appSidebarDrawer.visible
                    focusPolicy: Qt.NoFocus
                    icon.source: "qrc:/icon-more"
                    onClicked: {
                        if (appContextMenu.visible) appContextMenu.close()
                        else appContextMenu.popup(this, 0, height)
                    }
                }
            }
            RowLayout {
                ToolButton {
                    Layout.fillHeight: true
                    focusPolicy: Qt.NoFocus
                    icon.source: appLastDialog ? "qrc:/icon-pause" : "qrc:/icon-close"
                    action: appEscapeAction
                }
                Label {
                    text: HttpRequest.running ? HttpRequest.url.toString() :
                           (SshProcess.running ? Url.textAt(SshProcess.sshUrl) : SystemProcess.command)
                    elide: Text.ElideRight
                    TapHandler {
                        onTapped: appEscapeAction.trigger()
                    }
                }
            }
        }
        ProgressBar {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            visible: VMConfigSet.isBusy && !appLastDialog
            indeterminate: visible
        }
    }

    StackView {
        id: appStackView
        anchors.fill: parent
        focus: true // turn-on active focus here
        transform: Translate { x: appSidebarDrawer.position * appSidebarDrawer.width }
        onDepthChanged: VMConfigSet.clusterEnabled = (depth === 1)
        initialItem: AppMainPage { id: appMainPage }
    }
    function appPage(qml, prop = {}) { // prevent dups
        if (appStackView.find(function(item) { return (item.objectName === qml) })) return null
        var page = appStackView.push(qml, prop)
        if (!page) appToast(qsTr("Can't load %1").arg(qml))
        else if (page instanceof Page) page.objectName = qml
        else appToast(qsTr("Not a Page instance %1").arg(qml))
        return page
    }

    footer: ToolBar {
        visible: appPortraitView && appStackView.depth === 1
        enabled: !appSidebarDrawer.visible
        RowLayout {
            anchors.fill: parent
            spacing: 0
            ToolButtonTemplate { action: appStartAction }
            ToolButtonTemplate { action: appStopAction }
            ToolButtonTemplate { action: appTerminalAction }
            ToolButtonTemplate { action: appDesktopAction }
        }
    }

    component MenuItemTemplate: MenuItem {
        arrow: Text {
            visible: !isMobile && parent.enabled
            anchors { right: parent.right; rightMargin: parent.rightPadding; verticalCenter: parent.verticalCenter }
            font: parent.font
            text: SystemHelper.shortcutText(parent.action.shortcut)
            color: Material.foreground
        }
    }

    Menu {
        id: appContextMenu
        Material.background: MaterialSet.theme[Material.theme]["highlight"]
        Material.elevation: MaterialSet.theme[Material.theme]["elevation"]
        MenuItemTemplate { action: appFullScreenAction }
        MenuItemTemplate { action: appCompactAction }
        MenuSeparator {}

        property int fixedItems: 0
        Component.onCompleted: fixedItems = count
        onAboutToShow: {
            var i
            for (i = count - 1; i >= fixedItems; i--) {
                takeAction(i)
            }
            if (appStackView.currentItem && appStackView.currentItem.actionsList) {
                for (i = 0; i < appStackView.currentItem.actionsList.length; i++) {
                    addAction(appStackView.currentItem.actionsList[i])
                }
            }
        }
    }

    AppSidebarDrawer {
        id: appSidebarDrawer
        x: 0; y: appWindow.header.height
        width: Math.min(Math.round(appWindow.width / 2), 220)
        height: appWindow.contentItem.height
        interactive: appStackView.depth < 2
    }

    property var appLastDelay
    Component {
        id: delayComponent
        Timer {
            property var func
            property var args
            running: true
            onTriggered: {
                func(...args)
                destroy()
                appLastDelay = null
            }
        }
    }
    function appDelay(interval, func, ...args) { // return Timer instance
        if (appLastDelay) {
            appLastDelay.stop()
            appLastDelay.destroy()
        }
        appLastDelay = delayComponent.createObject(appWindow, { interval, func, args });
    }

    Component {
        id: toastComponent
        ToolTip {
            font.pointSize: appTipSize
            timeout: 2500
            visible: text
            onVisibleChanged: if (!visible) destroy()
        }
    }
    function appToast(text) {
        toastComponent.createObject(appWindow, { text })
    }
}
