pragma Singleton
import QtQuick 2.15

import CppCustomModules 1.0

Item {
    id: control

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    readonly property string cbsdName: "cbsd"
    property string cbsdPath: SystemHelper.userHome(cbsdName) ? SystemHelper.findExecutable(cbsdName) : ""
    property bool clusterEnabled: true
    property var currentConfig: ({})
    property int currentProgress: -1
    readonly property string sudoPswd: "Pswd"
    readonly property string sudoExec: "sudo -Sp" + sudoPswd
    readonly property string cbsdPrefix: Qt.platform.os === "unix" ? "b" : "q"
    property string cbsdProfiles: "get-profiles src=cloud"
    property string cbsdCluster:  "cluster"
    property string cbsdCreate:   cbsdPrefix + "create runasap=1 ci_ip4_addr=DHCP ci_gw4=10.0.0.1"
    property string cbsdStart:    cbsdPrefix + "start quiet=1"
    property string cbsdStop:     cbsdPrefix + "stop quiet=1"
    property string cbsdDestroy:  cbsdPrefix + "destroy"
    readonly property bool isBusy:    SystemProcess.running || HttpRequest.running
    readonly property bool isValid:   currentConfig.hasOwnProperty("server")
    readonly property bool isCreated: isValid && currentConfig.hasOwnProperty("id")
    readonly property bool isPowerOn: isCreated && (currentConfig["is_power_on"] === "true" || currentConfig["is_power_on"] === true)
    readonly property bool isSshUser: isPowerOn && currentConfig.hasOwnProperty("ssh_user")
    readonly property bool isRdpHost: isPowerOn && currentConfig.hasOwnProperty("rdp_user") && currentConfig.hasOwnProperty("rdp_host")
    readonly property bool isVncHost: isPowerOn && currentConfig.hasOwnProperty("vnc_host")

    readonly property string templateId: "vmTemplate"
    property string lastSelected: ""
    property int progressPeriod: 3 // seconds
    property int clusterPeriod: 60 // seconds
    readonly property int clusterDelay: 1000 // milliseconds

    signal message(string text)
    signal gotProfiles(string from, var array)
    signal progress(string text)

    onCurrentConfigChanged: {
        if (control.isValid) {
            var key = objectKey(currentConfig)
            if (key) {
                lastSelected = key
                return
            }
        }
        lastSelected = ""
    }
    //onLastSelectedChanged: console.debug("lastSelected", lastSelected)

    MySettings {
        category: control.myClassName
        property alias lastSelected: control.lastSelected
        property alias progressPeriod: control.progressPeriod
        property alias clusterPeriod: control.clusterPeriod

        property alias cbsdProfiles: control.cbsdProfiles
        property alias cbsdCluster:  control.cbsdCluster
        property alias cbsdCreate:   control.cbsdCreate
        property alias cbsdStart:    control.cbsdStart
        property alias cbsdStop:     control.cbsdStop
        property alias cbsdDestroy:  control.cbsdDestroy
    }

    Timer {
        interval: progressPeriod * 1000 // make milliseconds
        repeat: true
        running: interval >= 1000 && currentProgress >= 0
        triggeredOnStart: true
        onTriggered: {
            if (currentProgress < 0) {
                stop()
            } else if (control.isValid && !isCbsdExecutable(currentConfig["server"])) {
                RestApiSet.getStatus(currentConfig)
            }
        }
    }

    Timer {
        id: clusterTimer
        interval: clusterDelay
        repeat: true
        running: false
        onTriggered: {
            if (clusterEnabled && !control.isBusy && currentProgress < 0) {
                var period = Math.max(control.clusterPeriod, 30) * 1000 // make milliseconds
                if (interval !== period) {
                    interval = period
                    restart()
                }
                var folders = SystemHelper.fileList(null, null, true)
                if (!folders.length) {
                    var list = SystemHelper.loadSettings("VMProfilesPage/cloudServers", "").split(',').filter(Boolean)
                    if (list.length) folders.push(list[0])
                }
                for (var server of folders) {
                    if (isCbsdExecutable(server) || RestApiSet.isSshKeyExist(server))
                        getList(server)
                }
            } else if (interval !== clusterDelay) {
                interval = clusterDelay
                restart()
            }
        }
    }

    Connections {
        target: SystemProcess
        function onFinished(code) {
            message(cbsdPath + " return " + code)
            if (code || !cbsdPath) return
            var cmd = SystemProcess.command
            var pos = cmd.indexOf(cbsdName)
            if (pos < 0) return
            cmd = cmd.slice(pos + cbsdName.length + 1)
            if (cmd.startsWith(cbsdProfiles)) {
                gotProfiles(cbsdPath, SystemHelper.loadArray(cbsdName + "/profiles.json"))
                return
            }
            if (cmd.startsWith(cbsdCluster)) {
                parseCluster(cbsdPath, SystemHelper.loadObject(cbsdName + "/cluster.json"))
                return
            }
            if (cmd.startsWith(cbsdCreate)) {
                parseCreate(cbsdPath)
            } else if (!cmd.startsWith(cbsdStart) && !cmd.startsWith(cbsdStop) && !cmd.startsWith(cbsdDestroy)) {
                message("Unexpected command " + SystemProcess.command)
                return
            }
            currentProgress = -1
            clusterTimer.interval = clusterDelay
            clusterTimer.restart()
        }
    }

    Connections {
        target: RestApiSet
        function onResponse(cmd, host, name) {
            var file = SystemHelper.appDataDir + '/' + host + '/' + RestApiSet.cmdName(cmd)
            if (SystemHelper.isFile(file)) {
                switch (cmd) {
                case RestApiSet.Cmd.Profiles: gotProfiles(host, SystemHelper.loadArray(file)); return
                case RestApiSet.Cmd.Create:   parseCreate(host, SystemHelper.loadObject(file)); return
                case RestApiSet.Cmd.Cluster:  parseCluster(host, SystemHelper.loadObject(file)); return
                case RestApiSet.Cmd.Status:
                    if (!name) break
                    parseStatus(host, name, SystemHelper.loadObject(file))
                    return
                case RestApiSet.Cmd.Start:
                case RestApiSet.Cmd.Stop:
                    if (!name) break
                    parseStartStop(host, name, SystemHelper.loadObject(file))
                    return
                case RestApiSet.Cmd.Destroy:
                    if (SystemHelper.loadObject(file)["Message"] !== "destroy") break
                    return
                }
            }
            message(host + " unexpected response " + cmd)
        }
    }

    function start() {
        setCurrent(control.lastSelected)
        clusterTimer.start()
    }

    function isCbsdExecutable(name) : bool {
        return (cbsdPath && (name === cbsdName || name === cbsdPath))
    }

    function executeCbsd(args, file) {
        //console.debug("executeCbsd", args, file)
        if (!cbsdPath) return
        if (!file) SystemProcess.stdOutFile = ""
        else if (SystemHelper.isAbsolute(file)) SystemProcess.stdOutFile = file
        else SystemProcess.stdOutFile = SystemHelper.appDataPath(cbsdName) + '/' + file

        SystemProcess.command = sudoExec + ' ' + cbsdPath + ' ' + args
        Qt.callLater(SystemProcess.start)
        message(SystemProcess.command)
    }

    function getProfiles(from) {
        //console.debug("getProfiles", from)
        if (isCbsdExecutable(from)) {
            executeCbsd(cbsdProfiles + " json=1", "profiles.json")
        } else RestApiSet.getProfiles(from)
    }

    function getList(from) : bool {
        //console.debug("getList", from)
        if (isCbsdExecutable(from)) {
            executeCbsd(cbsdCluster, "cluster.json")
            return true
        }
        return RestApiSet.getCluster(from)
    }

    function createVm(cfg) : bool {
        //console.debug("createVm", cfg["server"])
        if (isCbsdExecutable(cfg["server"])) {
            var cmd = cbsdCreate + " inter=0 jname=" + cfg["alias"]

            var type = cfg["vm_os_type"] ? cfg["vm_os_type"] : cfg["type"]
            var profile = cfg["vm_os_profile"] ? cfg["vm_os_profile"] : cfg["profile"]
            cmd += " vm_os_type=" + type + " vm_os_profile=" + profile

            cmd += " imgsize="
            if (cfg["imgsize_bytes"]) cmd += cfg["imgsize_bytes"]
            else cmd += RestApiSet.defDiskSize + 'g'

            cmd += " vm_cpus="
            if (cfg["cpus"]) cmd += cfg["cpus"]
            else cmd += RestApiSet.defCpuCount

            cmd += " vm_ram="
            if (cfg["ram_bytes"]) cmd += cfg["ram_bytes"]
            else cmd += RestApiSet.defRamSize + 'g'

            currentProgress = 0
            progress(qsTr("Creating <b>%1</b>").arg(cfg["alias"]))
            SystemHelper.saveText(cbsdName + '/' + profile.replace(/\./g, '_'), cbsdPath + ' ' + cmd)
            executeCbsd(cmd)
            return true
        }
        return RestApiSet.postCreate(config)
    }

    function startCurrent() {
        if (control.isCreated) {
            if (isCbsdExecutable(currentConfig["server"])) {
                var cmd = cbsdStart + " inter=0 jname=" + currentConfig["id"]
                executeCbsd(cmd)
            } else {
                RestApiSet.getStart(currentConfig)
            }
        }
    }

    function stopCurrent() {
        if (control.isCreated) {
            if (isCbsdExecutable(currentConfig["server"])) {
                var cmd = cbsdStop + " jname=" + currentConfig["id"]
                executeCbsd(cmd)
            } else {
                RestApiSet.getStop(currentConfig)
            }
        }
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

    function desktopUrl() : url {
        var url = currentConfig.hasOwnProperty("desktop") ? currentConfig["desktop"].toLowerCase() : ""
        if (control.isRdpHost && (!url || url === "rdp")) {
            url = "rdp://"
            if (currentConfig.hasOwnProperty("rdp_user")) url += currentConfig["rdp_user"]
            else if (currentConfig.hasOwnProperty("id"))  url += currentConfig["id"]
            else url += Qt.application.name
            if (currentConfig.hasOwnProperty("rdp_password"))
                url += ':' + currentConfig["rdp_password"]
            if (!currentConfig.hasOwnProperty("rdp_host")) return null
            url += '@' + currentConfig["rdp_host"]
            if (currentConfig.hasOwnProperty("rdp_port"))
                url += ':' + currentConfig["rdp_port"]
            return url
        }
        if (control.isVncHost && (!url || url === "vnc")) {
            url = "vnc://"
            if (currentConfig.hasOwnProperty("vnc_user")) url += currentConfig["vnc_user"]
            else if (currentConfig.hasOwnProperty("id"))  url += currentConfig["id"]
            else url += Qt.application.name
            if (currentConfig.hasOwnProperty("vnc_password"))
                url += ':' + currentConfig["vnc_password"]
            if (!currentConfig.hasOwnProperty("vnc_host")) return null
            url += '@' + (isCbsdExecutable(currentConfig["server"]) ? "127.0.0.1" : currentConfig["vnc_host"])
            if (currentConfig.hasOwnProperty("vnc_port"))
                url += ':' + currentConfig["vnc_port"]
            return url
        }
        message("Desktop feature not available")
        return null
    }

    function setCurrent(at) {
        var map = configMap()
        if (at && map[at]) {
            currentConfig = Object.assign({}, map[at])
            return
        }
        for (var key in map) {
            currentConfig = Object.assign({}, map[key])
            return
        }
        currentConfig = {}
    }

    function updateCurrent(config, replace = false) : bool {
        var key = objectKey(config)
        if (!key) return false
        var conf_path = SystemHelper.fileName(config["server"]) + "/VirtualMachines"
        var map = SystemHelper.loadObject(conf_path)
        if (replace || !map[key]) map[key] = config
        else map[key] = Object.assign(map[key], config)
        if (!SystemHelper.saveObject(conf_path, map)) {
            message("updateCurrent can't write " + conf_path)
            return false
        }
        currentConfig = Object.assign({}, map[key])
        return true
    }

    function removeConfig(config) : bool {
        var key = objectKey(config)
        if (!key) return false
        var conf_path = SystemHelper.fileName(config["server"]) + "/VirtualMachines"
        var map = SystemHelper.loadObject(conf_path)
        if (map.hasOwnProperty(key)) {
            var obj = map[key]
            delete map[key]
            if (!SystemHelper.saveObject(conf_path, map)) {
                message("removeConfig can't write " + conf_path)
                return false
            }
            if (key === objectKey(currentConfig))
                setCurrent() // any first one or nothing

            if (isCbsdExecutable(obj["server"])) {
                executeCbsd(cbsdDestroy + " jname=" + obj["id"])
            } else {
                RestApiSet.getDestroy(obj)
            }
        }
        return true
    }

    function vmInstanceId(obj) : string {
        if (obj && typeof obj === "object") {
            if (obj.hasOwnProperty("id")) return obj["id"]
            if (obj.hasOwnProperty("instanceid")) return obj["instanceid"]
        }
        return undefined
    }

    function parseCluster(from, reply) {
        var list = reply.hasOwnProperty("servers") ? reply["servers"] : []
        if (!list.length) return
        var profiles = []
        var base = SystemHelper.fileName(from)
        var conf_path = base + "/VirtualMachines"
        var key, next_map = {}, curr_map = SystemHelper.loadObject(conf_path)
        for (var item of list) {
            if (typeof item !== "object" || Array.isArray(item)) continue
            key = vmInstanceId(item)
            if (!key) {
                message("The \"id\" of the Cluster response is missing " + from)
                return
            }
            key += '@' + base
            if (curr_map.hasOwnProperty(key)) {
                next_map[key] = Object.assign(curr_map[key], item)
                delete curr_map[key]
            } else next_map[key] = item

            if (!next_map[key].hasOwnProperty("alias"))  next_map[key]["alias"] = key
            if (!next_map[key].hasOwnProperty("server")) next_map[key]["server"] = from
            if (!next_map[key].hasOwnProperty("name")) {
                if (!profiles.length) profiles = SystemHelper.loadArray(base + "/profiles")
                var index = profiles.findIndex(obj => obj.profile === item["vm_os_profile"])
                next_map[key]["name"] = ~index ? profiles[index]["name"] : item["vm_os_profile"]
            }
        }
        var temp = control.templateId + '@'
        for (key in curr_map) {
            if (key.startsWith(temp)) next_map[key] = curr_map[key]
        }
        var keys = Object.keys(next_map)
        if (!keys.length) {
            console.debug("parseCluster: No next_map keys")
            return
        }
        if (!SystemHelper.saveObject(conf_path, next_map)) {
            message("parseCluster can't write " + conf_path)
            return
        }
        if (control.isValid) {
            key = objectKey(currentConfig)
            if (next_map.hasOwnProperty(key)) {
                currentConfig = Object.assign({}, next_map[key])
                return
            }
        }
        if (keys.length > 1) keys.sort()
        setCurrent(keys[0])
    }

    function parseCreate(from, reply) {
        var id = vmInstanceId(reply)
        var temp = control.templateId + '@'
        var base = SystemHelper.fileName(from)
        var conf_path = base + "/VirtualMachines"
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
            if (!obj.hasOwnProperty("server")) obj["server"] = from
            map[obj["id"] + '@' + base] = obj
            delete map[key]
            if (!SystemHelper.saveObject(conf_path, map)) {
                message("parseCreate can't write " + conf_path)
                continue
            }
            if (key === objectKey(currentConfig)) {
                currentConfig = Object.assign(currentConfig, obj)
                currentProgress = id ? 0 : -1
            } else message("Unexpected Create response " + key)
            break
        }
    }

    function parseStatus(from, name, reply) {
        var base = SystemHelper.fileName(from)
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
        var conf_path = base + "/VirtualMachines"
        var map = SystemHelper.loadObject(conf_path)
        map[key] = map.hasOwnProperty(key) ? Object.assign(map[key], reply) : reply
        if (!SystemHelper.saveObject(conf_path, map)) message("parseStatus can't write " + conf_path)
        else currentConfig = Object.assign(currentConfig, reply)
        currentProgress = -1
    }

    function parseStartStop(from, name, reply) {
        var base = SystemHelper.fileName(from)
        var key = name + '@' + base
        if (key !== objectKey(currentConfig, false)) {
            message("Unexpected Start/Stop response " + key)
            return
        }
        var msg = reply["Message"]
        if (msg === "started" || msg === "stopped") {
            currentConfig = Object.assign(currentConfig, reply)
            currentProgress = 0
        } else message("Unknown Start/Stop message " + from + ": " + msg)
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
            message("The \"id\" parameters is missing at " + config["name"])
        } else message("The \"server\" parameters is missing at " + config["name"])
        return null
    }

    function configMap() {
        var all = {}, folders = SystemHelper.fileList(null, null, true)
        for (var server of folders) {
            var map = SystemHelper.loadObject(server + "/VirtualMachines")
            for (var key in map) {
                var obj = map[key]
                if (typeof obj !== "object" || Array.isArray(obj)) {
                    message("Not an object at the key " + key)
                    continue
                }
                if (all.hasOwnProperty(key)) {
                    message("The object key is dupplicated " + key)
                    continue
                }
                all[key] = obj
            }
        }
        return all
    }

    function isUniqueValue(varName, value) {
        var temp = control.templateId + '@'
        var vlc = value.toLowerCase(), map = configMap()
        for (var key in map) {
            if (key.startsWith(temp)) continue
            var obj = map[key]
            if (obj.hasOwnProperty(varName) && obj[varName].toLowerCase() === vlc)
                return false
        }
        return true
    }

    function uniqueValue(varName, valPrefix) : string {
        var temp = control.templateId + '@'
        var sample = valPrefix.toLowerCase(), list = [], map = configMap()
        for (var key in map) {
            if (key.startsWith(temp)) continue
            var obj = map[key]
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
