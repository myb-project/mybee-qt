pragma Singleton
import QtQml 2.15

import CppCustomModules 1.0

QtObject {
    id: control

    enum Cmd { Invalid, Profiles, Create, Cluster, Status, Start, Stop, Destroy } // total: 8
    readonly property var cmdStrings: [
        "invalid", "profiles", "create", "cluster", "status", "start", "stop", "destroy" ]

    property string urlScheme: "http"
    readonly property string urlPrefix:    "/api/v1/"   // replace with actual API version
    readonly property string authConfFile: "AuthConfig.json"
    readonly property string sshPrivName:  "ssh_privkey"
    readonly property string sshPubName:   "ssh_pubkey"
    readonly property int defDiskSize:  16  // GB
    readonly property int defCpuCount:  1
    readonly property int defRamSize:   4   // GB

    signal message(string text)
    signal response(int cmd, string host, string name)

    Component.onCompleted: {
        var env = SystemHelper.envVariable("CLOUD_URL")
        urlScheme = (env && Url.isRemoteAt(env)) ? Url.schemeAt(env)
                                                 : SystemHelper.loadSettings("urlScheme", "http")

        HttpRequest.recvArray.connect(function(url, data) {
            var host = Url.hostAt(url)
            if (host) {
                var cmd = urlCmdIndex(url)
                var file = host + '/' + control.cmdStrings[cmd]
                if (SystemHelper.saveArray(file, data)) response(cmd, host, "")
                else message("Can't write " + file)
            } else message("No host at response " + url)
        })
        HttpRequest.recvObject.connect(function(url, data) {
            var host = Url.hostAt(url)
            if (host) {
                var name = "", cmd = urlCmdIndex(url)
                if (cmd !== RestApiSet.Cmd.Cluster) {
                    if (cmd >= RestApiSet.Cmd.Status)
                        name = SystemHelper.fileName(Url.pathAt(url))
                    var json = JSON.stringify(data, null, 4)
                    if (json) message(json)
                }
                var file = host + '/' + control.cmdStrings[cmd]
                if (SystemHelper.saveObject(file, data)) response(cmd, host, name)
                else message("Can't write " + file)
            } else message("No host at response " + url)
        })
    }

    // public functions

    function isSshKeyExist(host) : bool {
        var conf = host ? SystemHelper.loadObject(host + '/' + control.authConfFile) : {}
        if (!conf[control.sshPrivName]) conf = SystemHelper.loadObject(control.authConfFile)
        return (SystemHelper.isSshPrivateKey(conf[control.sshPrivName]) &&
                SystemHelper.sshPublicKey(conf[control.sshPubName]))
    }

    function cmdName(cmd) : string {
        return (cmd >= 0 && cmd < 8) ? control.cmdStrings[cmd] : "unknown";
    }

    function getProfiles(host) {
        //console.debug("getProfiles", host)
        var msg = "getProfiles: Unexpected error"
        if (host) {
            var url = control.urlScheme + "://" + host + '/' + control.cmdStrings[RestApiSet.Cmd.Profiles]
            if (HttpRequest.sendGet(url)) msg = url
        }
        message(msg)
    }

    function postCreate(cfg) : bool {
        //console.debug("postCreate", cfg["vm_os_profile"])
        var host = cfg["server"]
        var key = sshPublicKey(host)
        if (!key) return false
        var obj = {}
        obj["pubkey"] = key

        var type = cfg["vm_os_type"] ? cfg["vm_os_type"] : cfg["type"]
        var profile = cfg["vm_os_profile"] ? cfg["vm_os_profile"] : cfg["profile"]
        if (cfg["image"]) {
            obj["image"] = cfg["image"]
        } else {
            obj["vm_os_type"] = type
            obj["vm_os_profile"] = profile
        }

        if (cfg["imgsize_bytes"]) obj["imgsize"] = cfg["imgsize_bytes"]
        else obj["imgsize"] = defDiskSize + 'g'

        if (cfg["cpus"]) obj["cpus"] = cfg["cpus"]
        else obj["cpus"] = defCpuCount

        if (cfg["ram_bytes"]) obj["ram"] = cfg["ram_bytes"]
        else obj["ram"] = defRamSize + 'g'

        SystemHelper.saveObject(host + '/' + profile.replace(/\./g, '_'), obj)
        var url = control.urlScheme + "://" + host + control.urlPrefix
                + control.cmdStrings[RestApiSet.Cmd.Create] + "/_"
        if (HttpRequest.sendPost(url, obj)) message(url)
        else message("postCreate: Unexpected error")
        return true
    }

    function getCluster(host) : bool {
        //console.debug("getCluster", host)
        var key = sshPublicKey(host)
        if (!key) return false
        var url = control.urlScheme + "://" + host + control.urlPrefix
                + control.cmdStrings[RestApiSet.Cmd.Cluster]
        if (HttpRequest.sendGet(url, key)) message(url)
        else message("getCluster: Unexpected error")
        return true
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

    // private functions

    function sendKeyGet(cfg, cmd) {
        var msg = "sendKeyGet: Unexpected error"
        var host = cfg["server"]
        if (host && cfg["id"]) {
            var key = sshPublicKey(host)
            if (key) {
                var url = control.urlScheme + "://" + host + control.urlPrefix
                        + cmd + '/' + cfg["id"]
                if (HttpRequest.sendGet(url, key)) msg = url
            }
        }
        message(msg)
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

    function sshPublicKey(host) : string {
        var conf = host ? SystemHelper.loadObject(host + '/' + control.authConfFile) : {}
        if (!conf[control.sshPubName]) conf = SystemHelper.loadObject(control.authConfFile)
        if (conf[control.sshPubName]) {
            var key = SystemHelper.sshPublicKey(conf[control.sshPubName])
            if (key) return key
        }
        return null
    }
}
