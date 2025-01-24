pragma Singleton
import QtQml 2.15

import CppCustomModules 1.0

QtObject {
    id: control

    enum Cmd { Invalid, Profiles, Create, Cluster, Status, Start, Stop, Destroy } // total: 8
    readonly property var cmdStrings: [
        "invalid", "profiles", "create", "cluster", "status", "start", "stop", "destroy" ]

    readonly property string urlPrefix:    "/api/v1/"   // replace with actual API version
    readonly property int defDiskSize:  16  // GB
    readonly property int defCpuCount:  1
    readonly property int defRamSize:   4   // GB

    signal message(string text)
    signal error(string text)
    signal response(int cmd, string from, string name)

    Component.onCompleted: {
        HttpRequest.recvArray.connect(function(url, data) {
            var cmd = urlCmdIndex(url)
            if (!cmd) return
            var file = Url.hostAt(url) + '/' + control.cmdStrings[cmd]
            if (SystemHelper.saveArray(file, data))
                response(cmd, Url.adjustedAt(url), "")
            else error("Can't write " + file)
        })
        HttpRequest.recvObject.connect(function(url, data) {
            var cmd = urlCmdIndex(url)
            if (!cmd) return
            var name = ""
            if (cmd !== RestApiSet.Cmd.Cluster) {
                if (cmd >= RestApiSet.Cmd.Status)
                    name = SystemHelper.fileName(Url.pathAt(url))
                var json = JSON.stringify(data, null, 4)
                if (json) message(json)
            }
            var file = Url.hostAt(url) + '/' + control.cmdStrings[cmd]
            if (SystemHelper.saveObject(file, data))
                response(cmd, Url.adjustedAt(url), name)
            else error("Can't write " + file)
        })
    }

    function cmdName(cmd) : string {
        return (cmd >= 0 && cmd < 8) ? control.cmdStrings[cmd] : "unknown";
    }

    function getProfiles(cfg) {
        //console.debug("getProfiles", cfg["server"])
        var url = cfg["server"]
        if (url) {
            url += '/' + control.cmdStrings[RestApiSet.Cmd.Profiles]
            if (HttpRequest.sendGet(url)) {
                message(url)
                return
            }
        }
        error("getProfiles: Unexpected error")
    }

    function getCluster(cfg) {
        //console.debug("getCluster", cfg["server"])
        var key = SystemHelper.sshPublicKey(cfg["ssh_key"])
        if (!key) {
            error("getCluster: No ssh public key at " + cfg["ssh_key"])
            return
        }
        var url = cfg["server"]
        if (url) {
            url += control.urlPrefix + control.cmdStrings[RestApiSet.Cmd.Cluster]
            if (HttpRequest.sendGet(url, key)) {
                message(url)
                return
            }
        }
        error("getCluster: Unexpected error")
    }

    function postCreate(cfg) {
        //console.debug("postCreate", cfg["server"])
        var key = SystemHelper.sshPublicKey(cfg["ssh_key"])
        if (!key) {
            error("postCreate: No ssh public key at " + cfg["ssh_key"])
            return
        }
        var obj = {}
        obj["pubkey"] = key

        if (!cfg["image"]) {
            obj["vm_os_type"] = cfg["vm_os_type"] ? cfg["vm_os_type"] : cfg["type"]
            obj["vm_os_profile"] = cfg["vm_os_profile"] ? cfg["vm_os_profile"] : cfg["profile"]
        } else obj["image"] = cfg["image"]

        if (cfg["imgsize"]) obj["imgsize"] = cfg["imgsize"]
        else if (cfg["imgsize_bytes"]) obj["imgsize"] = cfg["imgsize_bytes"]
        else obj["imgsize"] = defDiskSize + 'g'

        if (cfg["ram"]) obj["ram"] = cfg["ram"]
        else if (cfg["ram_bytes"]) obj["ram"] = cfg["ram_bytes"]
        else obj["ram"] = defRamSize + 'g'

        if (cfg["cpus"]) obj["cpus"] = cfg["cpus"]
        else obj["cpus"] = defCpuCount

        var url = cfg["server"]
        if (url) {
            url += control.urlPrefix + control.cmdStrings[RestApiSet.Cmd.Create] + "/_"
            if (HttpRequest.sendPost(url, obj)) {
                message(url)
                return
            }
        }
        error("postCreate: Unexpected error")
    }

    function getStatus(cfg) {
        //console.debug("getStatus", cfg["server"])
        sendKeyGet(cfg, control.cmdStrings[RestApiSet.Cmd.Status])
    }

    function getStart(cfg) {
        //console.debug("getStart", cfg["server"])
        sendKeyGet(cfg, control.cmdStrings[RestApiSet.Cmd.Start])
    }

    function getStop(cfg) {
        //console.debug("getStop", cfg["server"])
        sendKeyGet(cfg, control.cmdStrings[RestApiSet.Cmd.Stop])
    }

    function getDestroy(cfg) {
        //console.debug("getDestroy", cfg["server"])
        sendKeyGet(cfg, control.cmdStrings[RestApiSet.Cmd.Destroy])
    }

    function sendKeyGet(cfg, cmd) {
        var key = SystemHelper.sshPublicKey(cfg["ssh_key"])
        if (!key) {
            error("sendKeyGet: No ssh public key at " + cfg["ssh_key"])
            return
        }
        var url = cfg["server"]
        if (url && cfg["id"]) {
            url += control.urlPrefix + cmd + '/' + cfg["id"]
            if (HttpRequest.sendGet(url, key)) {
                message(url)
                return
            }
        }
        error("sendKeyGet: Unexpected error")
    }

    function urlCmdIndex(url) : int {
        var path = Url.pathAt(url)
        if (path) {
            for (var i = 0; i < control.cmdStrings.length; i++) {
                if (path.includes('/' + control.cmdStrings[i])) return i
            }
        }
        return 0
    }
}
