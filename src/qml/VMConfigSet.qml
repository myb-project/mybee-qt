pragma Singleton
import QtQuick 2.15
import QtQuick.Controls 2.15

import CppCustomModules 1.0

Item {
    id: control

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    readonly property string configFile: "VirtualMachines.json"
    readonly property string cbsdName: "cbsd"
    readonly property string cbsdHome: SystemHelper.userHome(cbsdName)
    readonly property bool cbsdEnabled: cbsdHome && SystemHelper.groupMembers(cbsdName).includes(SystemHelper.userName)
    //readonly property string cbsdSshKey: cbsdEnabled ? cbsdHome + "/.ssh/id_rsa" : ""
    property string cbsdPath: cbsdEnabled ? SystemHelper.findExecutable(cbsdName) : ""
    property var configMap: ({})
    property int configCount: 0
    property var currentConfig: ({})
    property int currentProgress: -1
    property bool clusterEnabled: false

    readonly property string defCbsdSshExec: "sudo " + cbsdName
    property string cbsdSshExec: defCbsdSshExec

    readonly property string sudoPswd: "Password:"
    readonly property string sudoExec: "sudo -Sp" + sudoPswd

    readonly property string defCbsdCapabilities: "capabilities json=1"
    readonly property string defCbsdProfiles: "get-profiles src=cloud json=1"
    readonly property string defCbsdCluster:  "cluster"
    readonly property string defCbsdCreate:   "create runasap=1 ci_ip4_addr=DHCP ci_gw4=10.0.0.1"
    readonly property string defCbsdStart:    "start quiet=1 inter=0"
    readonly property string defCbsdStop:     "stop quiet=1"
    readonly property string defCbsdDestroy:  "destroy"
    property string cbsdCapabilities: defCbsdCapabilities
    property string cbsdProfiles: defCbsdProfiles
    property string cbsdCluster:  defCbsdCluster
    property string cbsdCreate:   defCbsdCreate
    property string cbsdStart:    defCbsdStart
    property string cbsdStop:     defCbsdStop
    property string cbsdDestroy:  defCbsdDestroy

    readonly property bool isBusy:    HttpRequest.running || SshProcess.running || SystemProcess.running
    readonly property bool isValid:   currentConfig.hasOwnProperty("server")
    readonly property bool isCreated: isValid && currentConfig.hasOwnProperty("id")
    readonly property bool isPowerOn: isCreated && (currentConfig["is_power_on"] === "true" || currentConfig["is_power_on"] === true)
    readonly property bool isSshUser: isPowerOn && currentConfig.hasOwnProperty("ssh_user") && currentConfig.hasOwnProperty("ssh_key")
    readonly property bool isRdpHost: isPowerOn && currentConfig.hasOwnProperty("rdp_user") && currentConfig.hasOwnProperty("rdp_host")
    readonly property bool isVncHost: isPowerOn && currentConfig.hasOwnProperty("vnc_host")

    readonly property string templateId: "vmTemplate"
    property string lastSelected: ""
    property int clusterPeriod: 60 // seconds
    property int progressPeriod: 3 // seconds
    readonly property int clusterDelay: 1000 // milliseconds

    signal message(string text)
    signal error(string text)
    signal parseCapabilities(string url, var array)
    signal parseProfiles(string url, var array)
    signal listDone(string url)

    onCurrentConfigChanged: {
        if (configCount) {
            var key = objectKey(currentConfig)
            if (key) lastSelected = key
        } else lastSelected = ""
    }

    MySettings {
        category: control.myClassName
        property alias lastSelected: control.lastSelected

        property alias clusterPeriod:  control.clusterPeriod
        property alias progressPeriod: control.progressPeriod

        property alias cbsdCapabilities: control.cbsdCapabilities
        property alias cbsdProfiles: control.cbsdProfiles
        property alias cbsdCluster:  control.cbsdCluster
        property alias cbsdCreate:   control.cbsdCreate
        property alias cbsdStart:    control.cbsdStart
        property alias cbsdStop:     control.cbsdStop
        property alias cbsdDestroy:  control.cbsdDestroy
    }

    function setDefaults() {
        clusterPeriod  = 60
        progressPeriod = 3
        cbsdSshExec  = defCbsdSshExec
        cbsdCapabilities = defCbsdCapabilities
        cbsdProfiles = defCbsdProfiles
        cbsdCluster  = defCbsdCluster
        cbsdCreate   = defCbsdCreate
        cbsdStart    = defCbsdStart
        cbsdStop     = defCbsdStop
        cbsdDestroy  = defCbsdDestroy
    }

    Component.onCompleted: {
        // fix old settings from previous version
        var prefix = (Qt.platform.os === "unix") ? "b" : "q"
        if (cbsdCreate[0] === prefix) cbsdCreate = cbsdCreate.slice(1)
        if (cbsdStart[0] === prefix) cbsdStart = cbsdStart.slice(1)
        if (cbsdStop[0] === prefix) cbsdStop = cbsdStop.slice(1)
        if (cbsdDestroy[0] === prefix) cbsdDestroy = cbsdDestroy.slice(1)

        setConfigMap()
        setCurrent(control.lastSelected)
    }

    Timer {
        interval: progressPeriod * 1000 // make milliseconds
        repeat: true
        running: interval >= 1000 && currentProgress >= 0
        triggeredOnStart: true
        onTriggered: {
            if (currentProgress < 0) {
                stop()
            } else if (control.isValid && currentConfig["server"].startsWith("http")) {
                RestApiSet.getStatus(currentConfig)
            }
        }
    }

    Timer {
        id: clusterTimer
        interval: clusterDelay
        repeat: true
        running: true
        onTriggered: {
            if (clusterEnabled && !control.isBusy && currentProgress < 0) {
                var period = Math.max(control.clusterPeriod, 30) * 1000 // make milliseconds
                if (interval !== period) {
                    interval = period
                    restart()
                }
                var list = SystemHelper.fileList(null, null, true)
                for (var folder of list) {
                    var cfg = SystemHelper.loadObject(folder + "/lastServer")
                    if (cfg.hasOwnProperty("server") && cfg.hasOwnProperty("ssh_key"))
                        getList(cfg)
                }
            } else if (interval !== clusterDelay) {
                interval = clusterDelay
                restart()
            }
        }
    }

    Connections {
        target: HttpRequest
        function onCanceled() { currentProgress = -1 }
    }

    Connections {
        target: RestApiSet
        function onResponse(cmd, url, name) {
            var file = Url.hostAt(url) + '/' + RestApiSet.cmdName(cmd)
            switch (cmd) {
            case RestApiSet.Cmd.Profiles:
                parseProfiles(url, SystemHelper.loadArray(file))
                return
            case RestApiSet.Cmd.Create:
                parseCreate(url, SystemHelper.loadObject(file))
                return
            case RestApiSet.Cmd.Cluster:
                parseCluster(url, SystemHelper.loadObject(file))
                return
            case RestApiSet.Cmd.Status:
                if (!name) break
                parseStatus(url, name, SystemHelper.loadObject(file))
                return
            case RestApiSet.Cmd.Start:
            case RestApiSet.Cmd.Stop:
                if (!name) break
                parseStartStop(url, name, SystemHelper.loadObject(file))
                return
            case RestApiSet.Cmd.Destroy:
                if (SystemHelper.loadObject(file)["Message"] !== "destroy") break
                return
            }
            message(url + " unexpected response " + cmd)
        }
    }

    Connections {
        target: SshProcess
        function onCanceled() { currentProgress = -1 }
        function onFinished(code) {
            message(SshProcess.sshUrl.toString() + " return " + code)
            if (!code) {
                parseCbsdOutput(SshProcess.command, Url.textAt(SshProcess.sshUrl), Url.hostAt(SshProcess.sshUrl))
            }
        }
    }

    Connections {
        target: SystemProcess
        function onCanceled() { currentProgress = -1 }
        function onFinished(code) {
            message(cbsdPath + " return " + code)
            if (!code) {
                parseCbsdOutput(SystemProcess.command, "file:" + cbsdPath, cbsdName)
            }
        }
    }

    function parseCbsdOutput(command, url, dir) {
        //console.debug("parseCbsdOutput command:", command, "url:", url, "dir:", dir)
        var cmd = command
        var pos = cmd.indexOf(cbsdName + ' ')
        if (pos < 0) return
        cmd = cmd.slice(pos + cbsdName.length + 1)
        if (cmd.startsWith(cbsdCapabilities)) {
            parseCapabilities(url, SystemHelper.loadArray(dir + "/capabilities.json"))
            return
        }
        if (cmd.startsWith(cbsdProfiles)) {
            parseProfiles(url, SystemHelper.loadArray(dir + "/profiles.json"))
            return
        }
        if (cmd.startsWith(cbsdCluster)) {
            parseCluster(url, SystemHelper.loadObject(dir + "/cluster.json"))
            return
        }
        cmd = cmd.slice(1) // skip cbsdPrefix
        if (cmd.startsWith(cbsdCreate)) {
            parseCreate(url)
        } else if (!cmd.startsWith(cbsdStart) && !cmd.startsWith(cbsdStop) && !cmd.startsWith(cbsdDestroy)) {
            message("Unexpected command " + command)
            return
        }
        currentProgress = -1
        clusterTimer.interval = clusterDelay
        clusterTimer.restart()
    }

    function executeSsh(cfg, args, file) {
        //console.debug("executeSsh", args, file)
        var url = cfg["server"]
        if (!url || !SystemHelper.isSshPrivateKey(cfg["ssh_key"])) {
            error("The ssh server and/or private key are not configured")
            return
        }
        SshProcess.cancel()

        if (!file) SshProcess.stdOutFile = ""
        else if (SystemHelper.isAbsolute(file)) SshProcess.stdOutFile = file
        else SshProcess.stdOutFile = SystemHelper.appDataPath(Url.hostAt(url)) + '/' + file

        SshProcess.command = "env NOCOLOR=1 " + cbsdSshExec + ' ' + args
        SshProcess.start(url, cfg["ssh_key"])
        message(url + '?' + SshProcess.command)
    }

    function executeCbsd(args, file) {
        //console.debug("executeCbsd", args, file)
        if (!cbsdPath) {
            error("The path to cbsd is not configured")
            return
        }
        if (!file) SystemProcess.stdOutFile = ""
        else if (SystemHelper.isAbsolute(file)) SystemProcess.stdOutFile = file
        else SystemProcess.stdOutFile = SystemHelper.appDataPath(cbsdName) + '/' + file

        SystemProcess.command = sudoExec + ' ' + cbsdPath + ' ' + args
        Qt.callLater(SystemProcess.start)
        message(SystemProcess.command)
    }

    function isCapabilities(cfg) : bool {
        var scheme = Url.schemeAt(cfg["server"])
        return (scheme === "ssh" || scheme === "file")
    }

    function isSchemeEnabled(scheme) : bool {
        switch (scheme) {
        case "http":    return true
        case "https":   return HttpRequest.sslAvailable
        case "ssh":     return HttpRequest.sslAvailable
        case "file":    return cbsdEnabled
        }
        return false
    }

    function getCapabilities(cfg) {
        var scheme = Url.schemeAt(cfg["server"])
        if (isSchemeEnabled(scheme)) {
            switch (scheme) {
            case "ssh":
                executeSsh(cfg, cbsdCapabilities, "capabilities.json")
                return
            case "file":
                executeCbsd(cbsdCapabilities, "capabilities.json")
                return
            }
        }
        error("getCapabilities: Unsupported URL scheme: " + scheme)
    }

    function getProfiles(cfg) {
        //for (var key in cfg) print(key, cfg[key])
        var scheme = Url.schemeAt(cfg["server"])
        if (isSchemeEnabled(scheme)) {
            switch (scheme) {
            case "http":
            case "https":
                RestApiSet.getProfiles(cfg)
                return
            case "ssh":
                executeSsh(cfg, cbsdProfiles, "profiles.json")
                return
            case "file":
                executeCbsd(cbsdProfiles, "profiles.json")
                return
            }
        }
        error("getProfiles: Unsupported URL scheme: " + scheme)
    }

    function getList(cfg) {
        var scheme = Url.schemeAt(cfg["server"])
        if (isSchemeEnabled(scheme)) {
            switch (scheme) {
            case "http":
            case "https":
                RestApiSet.getCluster(cfg)
                return
            case "ssh":
                executeSsh(cfg, cbsdCluster, "cluster.json")
                return
            case "file":
                executeCbsd(cbsdCluster, "cluster.json")
                return
            }
        }
        error("getList: Unsupported URL scheme: " + scheme)
    }

    function cbsdPrefix(cfg) : string {
        if (cfg["prefix"]) return cfg["prefix"]
        if (cfg["emulator"]) return cfg["emulator"][0]
        return (Qt.platform.os === "unix" ? "b" : "q")
    }

    function createVm(cfg) {
        if (control.isCreated) return // just for sanity
        var scheme = Url.schemeAt(cfg["server"])
        if (isSchemeEnabled(scheme)) {
            switch (scheme) {
            case "http":
            case "https":
                RestApiSet.postCreate(cfg)
                return
            case "ssh":
            case "file":
                var key = SystemHelper.sshPublicKey(cfg["ssh_key"] + ".pub")
                if (!key) {
                    error("createVm: No public ssh key at " + cfg["ssh_key"])
                    return
                }
                var cmd = cbsdPrefix(cfg) + cbsdCreate + " inter=0 jname=" + cfg["alias"]
                cmd += " vm_os_type=" + (cfg["vm_os_type"] ? cfg["vm_os_type"] : cfg["type"])
                cmd += " vm_os_profile=" + (cfg["vm_os_profile"] ? cfg["vm_os_profile"] : cfg["profile"])

                cmd += " imgsize="
                if (cfg["imgsize"]) cmd += cfg["imgsize"]
                else if (cfg["imgsize_bytes"]) cmd += cfg["imgsize_bytes"]
                else cmd += RestApiSet.defDiskSize + 'g'

                cmd += " vm_ram="
                if (cfg["ram"]) cmd += cfg["ram"]
                else if (cfg["ram_bytes"]) cmd += cfg["ram_bytes"]
                else cmd += RestApiSet.defRamSize + 'g'

                cmd += " vm_cpus="
                if (cfg["cpus"]) cmd += cfg["cpus"]
                else cmd += RestApiSet.defCpuCount

                currentProgress = 0
                cmd += " app_frontend=mybqt ci_user_pubkey='" + key + "'"
                if (scheme === "ssh") executeSsh(cfg, cmd)
                else executeCbsd(cmd)
                return
            }
        }
        error("createVm: Unsupported URL scheme: " + scheme)
    }

    function removeVm() {
        var key = objectKey(currentConfig)
        if (!key) {
            error("removeVm: Object not found")
            return
        }
        var conf_path = SystemHelper.fileName(currentConfig["server"]) + '/' + configFile
        var map = SystemHelper.loadObject(conf_path)
        if (!map.hasOwnProperty(key)) {
            error("removeVm: Config not found")
            return
        }
        var obj = map[key]
        delete map[key]
        if (!SystemHelper.saveObject(conf_path, map)) {
            error("removeVm: Can't write " + conf_path)
            return
        }
        setConfigMap()
        if (key === objectKey(currentConfig))
            setCurrent() // any first one or nothing

        var scheme = Url.schemeAt(obj["server"])
        if (isSchemeEnabled(scheme)) {
            switch (scheme) {
            case "http":
            case "https":
                RestApiSet.getDestroy(obj)
                return
            case "ssh":
                executeSsh(obj, cbsdPrefix(obj) + cbsdDestroy + " jname=" + obj["id"])
                return
            case "file":
                executeCbsd(cbsdPrefix(obj) + cbsdDestroy + " jname=" + obj["id"])
                return
            }
        }
        error("removeVm: Unsupported URL scheme: " + scheme)
    }

    function startVm(cfg) {
        if (!cfg || !cfg.hasOwnProperty("server") || !cfg.hasOwnProperty("id")) {
            message("The \"server\" and/or \"id\" parameters is missing at " + cfg["name"])
            return
        }
        var scheme = Url.schemeAt(cfg["server"])
        if (isSchemeEnabled(scheme)) {
            switch (scheme) {
            case "http":
            case "https":
                RestApiSet.getStart(cfg)
                return
            case "ssh":
                executeSsh(cfg, cbsdPrefix(cfg) + cbsdStart + " jname=" + cfg["id"])
                return
            case "file":
                executeCbsd(cbsdPrefix(cfg) + cbsdStart + " jname=" + cfg["id"])
                return
            }
        }
        error("startVm: Unsupported URL scheme: " + scheme)
    }

    function stopVm(cfg) {
        if (!cfg || !cfg.hasOwnProperty("server") || !cfg.hasOwnProperty("id")) {
            message("The \"server\" and/or \"id\" parameters is missing at " + cfg["name"])
            return
        }
        var scheme = Url.schemeAt(cfg["server"])
        if (isSchemeEnabled(scheme)) {
            switch (scheme) {
            case "http":
            case "https":
                RestApiSet.getStop(cfg)
                return
            case "ssh":
                executeSsh(cfg, cbsdPrefix(cfg) + cbsdStop + " jname=" + cfg["id"])
                return
            case "file":
                executeCbsd(cbsdPrefix(cfg) + cbsdStop + " jname=" + cfg["id"])
                return
            }
        }
        error("stopVm: Unsupported URL scheme: " + scheme)
    }

    /*function parseCbsdList(qls) {
        var hdr = [], list = []
        for (var line of qls) {
            if (!hdr.length) line = line.toLowerCase()
            var seq = line.split(' ').filter(Boolean)
            if (!seq.length) continue
            if (!hdr.length) hdr = seq
            else if (seq.length === hdr.length) {
                var obj = {}
                for (var i = 0; i < seq.length; i++) {
                    obj[hdr[i]] = /^\d+$/.test(seq[i]) ? parseInt(seq[i]) : seq[i]
                    switch (hdr[i]) {
                    case "jname": obj["id"] = seq[i]; break
                    case "vm_ram": obj["ram"] = Math.round(parseInt(seq[i]) / 1024) + 'g'; break
                    case "vm_cpus": obj["cpus"] = parseInt(seq[i]); break
                    case "status": obj["is_power_on"] = seq[i].toLowerCase() === "on"; break
                    case "vnc_port": if (parseInt(seq[i])) obj["vnc_host"] = "127.0.0.1"
                    }
                }
                list.push(obj)
            }
        }
        SystemHelper.saveArray(cbsdName + "/qls.json", list)
        parseCluster(cbsdPath, list)
    }*/

    /*function desktopUrl() : url {
        var url = currentConfig.hasOwnProperty("desktop") ? currentConfig["desktop"].toLowerCase() : ""
        if (control.isRdpHost && (!url || url === "rdp")) {
            url = "rdp://"
            if (currentConfig.hasOwnProperty("rdp_user")) url += currentConfig["rdp_user"]
            else if (currentConfig.hasOwnProperty("id"))  url += currentConfig["id"]
            else url += Qt.application.name
            if (currentConfig.hasOwnProperty("rdp_password")) url += ':' + currentConfig["rdp_password"]
            if (!currentConfig.hasOwnProperty("rdp_host")) return null
            url += '@' + (Url.schemeAt(currentConfig["server"]) === "file" ? "127.0.0.1" : currentConfig["rdp_host"])
            if (currentConfig.hasOwnProperty("rdp_port")) url += ':' + currentConfig["rdp_port"]
            return url
        }
        if (control.isVncHost && (!url || url === "vnc")) {
            url = "vnc://"
            if (currentConfig.hasOwnProperty("vnc_user")) url += currentConfig["vnc_user"]
            else if (currentConfig.hasOwnProperty("id"))  url += currentConfig["id"]
            else url += Qt.application.name
            if (currentConfig.hasOwnProperty("vnc_password")) url += ':' + currentConfig["vnc_password"]
            if (!currentConfig.hasOwnProperty("vnc_host")) return null
            url += '@' + (Url.schemeAt(currentConfig["server"]) === "file" ? "127.0.0.1" : currentConfig["vnc_host"])
            if (currentConfig.hasOwnProperty("vnc_port")) url += ':' + currentConfig["vnc_port"]
            return url
        }
        error("Desktop feature not available")
        return null
    }*/

    function setConfigMap() {
        var map = {}, cnt = 0, list = SystemHelper.fileList(null, null, true)
        for (var folder of list) {
            var cfg = SystemHelper.loadObject(folder + '/' + configFile)
            for (var key in cfg) {
                var obj = cfg[key]
                if (typeof obj !== "object" || Array.isArray(obj)) {
                    message("setConfigMap: Not an object at the key: " + key)
                    continue
                }
                if (map.hasOwnProperty(key)) {
                    message("setConfigMap: The object key is dupplicated: " + key)
                    continue
                }
                var scheme = Url.schemeAt(obj["server"])
                if (!isSchemeEnabled(scheme)) {
                    message("setConfigMap: Unsupported URL scheme: " + scheme)
                    continue
                }
                map[key] = obj
                cnt++
            }
        }
        configMap = Object.assign({}, map)
        configCount = cnt
    }

    function setCurrent(at) {
        if (configMap.hasOwnProperty(at)) {
            if (configMap[at]["alias"] !== currentConfig["alias"])
                currentConfig = Object.assign({}, configMap[at])
            return
        }
        for (var key in configMap) { // to set the first existing one
            currentConfig = Object.assign({}, configMap[key])
            return
        }
        currentConfig = {}
    }

    function updateCurrent(config, replace = false) : bool {
        var key = objectKey(config)
        if (!key) return false
        var conf_path = SystemHelper.fileName(config["server"]) + '/' + configFile
        var map = SystemHelper.loadObject(conf_path)
        if (replace || !map[key]) map[key] = config
        else map[key] = Object.assign(map[key], config)
        if (!SystemHelper.saveObject(conf_path, map)) {
            error("updateCurrent: Can't write " + conf_path)
            return false
        }
        setConfigMap()
        currentConfig = Object.assign({}, map[key])
        return true
    }

    function vmInstanceId(obj) : string {
        if (obj && typeof obj === "object") {
            if (obj.hasOwnProperty("id")) return obj["id"]
            if (obj.hasOwnProperty("instanceid")) return obj["instanceid"]
        }
        return undefined
    }

    function parseCluster(url, reply) {
        var list = reply.hasOwnProperty("servers") ? reply["servers"] : []
        if (list.length) {
            var profiles = [], ssh_key = ""
            var base = SystemHelper.fileName(url)
            var conf_path = base + '/' + configFile
            var key, next_map = {}, curr_map = SystemHelper.loadObject(conf_path)
            for (var item of list) {
                if (typeof item !== "object" || Array.isArray(item)) {
                    message("The item of the Cluster response is not object " + url)
                    continue
                }
                key = vmInstanceId(item)
                if (!key) {
                    message("The \"id\" of the Cluster response is missing " + url)
                    continue
                }
                key += '@' + base
                if (curr_map.hasOwnProperty(key)) {
                    next_map[key] = Object.assign(curr_map[key], item)
                    delete curr_map[key]
                } else next_map[key] = item

                if (!next_map[key].hasOwnProperty("alias"))  next_map[key]["alias"] = key
                if (!next_map[key].hasOwnProperty("server")) next_map[key]["server"] = url
                if (!next_map[key].hasOwnProperty("name")) {
                    if (!profiles.length) profiles = SystemHelper.loadArray(base + "/profiles")
                    var index = profiles.findIndex(obj => obj.profile === item["vm_os_profile"])
                    next_map[key]["name"] = ~index ? profiles[index]["name"] : item["vm_os_profile"]
                }
                if (!next_map[key].hasOwnProperty("ssh_key")) {
                    if (!ssh_key) {
                        var obj = SystemHelper.loadObject(base + "/lastServer")
                        if (obj["ssh_key"]) ssh_key = obj["ssh_key"]
                        //else if (cbsdSshKey) ssh_key = cbsdSshKey
                        else ssh_key = SystemHelper.appSshKey
                    }
                    if (ssh_key) next_map[key]["ssh_key"] = ssh_key
                }
            }
            var temp = control.templateId + '@'
            for (key in curr_map) {
                if (key.startsWith(temp)) next_map[key] = curr_map[key]
            }
            var keys = Object.keys(next_map)
            if (!keys.length) {
                message("parseCluster: No next_map keys")
                return
            }
            if (!SystemHelper.saveObject(conf_path, next_map)) {
                error("parseCluster: Can't write " + conf_path)
                return
            }
            setConfigMap()
            if (control.isValid) {
                key = objectKey(currentConfig)
                if (next_map.hasOwnProperty(key)) {
                    currentConfig = Object.assign({}, next_map[key])
                    listDone(url)
                    return
                }
            }
            if (keys.length > 1) keys.sort()
            setCurrent(keys[0])
        }
        listDone(url)
    }

    function parseCreate(url, reply) {
        var id = vmInstanceId(reply)
        var temp = control.templateId + '@'
        var base = SystemHelper.fileName(url)
        var conf_path = base + '/' + configFile
        var map = SystemHelper.loadObject(conf_path)
        for (var key in map) {
            if (!key.startsWith(temp) || map[key].hasOwnProperty("id"))
                continue
            var pair = key.split('@')
            if (pair.length !== 2 || pair[1] !== base) {
                message("Found broken config key at " + key)
                continue
            }
            var obj = map[key]
            if (id) obj["id"] = id
            else if (obj["alias"]) obj["id"] = obj["alias"]
            else continue
            if (!obj.hasOwnProperty("server")) obj["server"] = url
            map[obj["id"] + '@' + base] = obj
            delete map[key]
            if (!SystemHelper.saveObject(conf_path, map)) {
                error("parseCreate: Can't write " + conf_path)
                continue
            }
            setConfigMap()
            if (key === objectKey(currentConfig)) {
                currentConfig = Object.assign(currentConfig, obj)
                currentProgress = id ? 0 : -1
            } else message("Unexpected Create response " + key)
            break
        }
    }

    function parseStatus(url, name, reply) {
        var base = SystemHelper.fileName(url)
        var key = name + '@' + base
        if (key !== objectKey(currentConfig, false)) {
            message("Unexpected Status response " + key)
            return
        }
        if (vmInstanceId(reply) !== name) {
            message("The Status response \"id\" missmatch request " + key)
            return
        }
        var pending = reply["status"] === "pending"
        if (pending) {
            if (reply.hasOwnProperty("progress")) {
                var progress = parseInt(reply["progress"])
                if (progress >= 0 && progress < 100) {
                    currentProgress = progress
                    return
                }
                delete reply["progress"]
            }
            delete reply["status"]
        }
        var conf_path = base + '/' + configFile
        var map = SystemHelper.loadObject(conf_path)
        map[key] = map.hasOwnProperty(key) ? Object.assign(map[key], reply) : reply
        if (SystemHelper.saveObject(conf_path, map)) {
            setConfigMap()
            currentConfig = Object.assign(currentConfig, reply)
        } else error("parseStatus: Can't write " + conf_path)
        currentProgress = -1
    }

    function parseStartStop(url, name, reply) {
        var base = SystemHelper.fileName(url)
        var key = name + '@' + base
        if (key !== objectKey(currentConfig, false)) {
            message("Unexpected Start/Stop response " + key)
            return
        }
        var msg = reply["Message"]
        if (msg === "started" || msg === "stopped") {
            currentConfig = Object.assign(currentConfig, reply)
            currentProgress = 0
        } else message("Unknown Start/Stop message " + url + ": " + msg)
    }

    function valueAt(varName) : string {
        return currentConfig.hasOwnProperty(varName) ? currentConfig[varName] : ""
    }

    function objectKey(config, template = true) : string {
        if (config.hasOwnProperty("server")) {
            var base = SystemHelper.fileName(config["server"])
            var id = vmInstanceId(config)
            if (template) return (id ? id : control.templateId) + '@' + base
            if (id) return id + '@' + base
            message("The \"id\" parameter is missing at " + config["name"])
        } else message("The \"server\" parameter is missing at " + config["name"])
        return null
    }

    function isUniqueValue(varName, value) {
        var temp = control.templateId + '@'
        var vlc = value.toLowerCase()
        for (var key in configMap) {
            if (key.startsWith(temp)) continue
            var obj = configMap[key]
            if (!obj.hasOwnProperty(varName)) continue
            var olc = obj[varName].toLowerCase()
            if (olc === vlc || olc === vlc + '@' + SystemHelper.fileName(obj["server"]))
                return false
        }
        return true
    }

    function uniqueValue(varName, valPrefix) : string {
        var temp = control.templateId + '@'
        var sample = valPrefix.toLowerCase(), list = []
        for (var key in configMap) {
            if (key.startsWith(temp)) continue
            var obj = configMap[key]
            if (obj.hasOwnProperty(varName) && obj[varName].toLowerCase().startsWith(sample)) {
                list.push(obj[varName])
                if (list.length > 1) list.sort()
            }
        }
        var last = 0
        if (list.length) {
            var str = list[list.length - 1], regex = /\d+/
            if (regex.test(str)) {
                var match = regex.exec(str)
                last = parseInt(match[0])
            }
        }
        last++
        return valPrefix + last.toString()
    }
}
