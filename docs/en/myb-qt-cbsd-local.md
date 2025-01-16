# Operating mode N1: local, via CBSD interpreter

This mode allows you to run virtual machines locally on those platforms where the [CBSD](https://github.com/cbsd/cbsd) software is installed. At the moment, these are platforms based on Linux, FreeBSD, XigmaNAS, DragonflyBSD and TrueNAS CORE.

The closest analogue of this mode of operation is Oracle VirtualBox.

If your working system is a mobile device, MacOS or Windows, two other methods will suit you ( [Operation mode N2: via SSH protocol, CBSD interpreter/MyBee](myb-qt-cbsd-ssh.md) or [Operation mode N3: via RestAPI/MyBee](myb-qt-api.md) ).

In this mode, MyBee-QT interacts directly with the /usr/local/bin/cbsd interpreter, so you will need to install additional software and configure access rights in advance.

<details>
  <summary>FreeBSD setup</summary>

1) You must have the CBSD package and its dependencies installed on your system:

```
pkg install -y cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

2) CBSD initialization:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) The user you run the application as must be a member of the 'cbsd' group.
```
pw group mod cbsd -m xuser
```
, where `xuser` - login of your user.

Please re-login back in to apply the change.

3) Your user must have permission to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration.

To do this, create a file */usr/local/etc/sudoers.d/20_cbsd* with the following contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `xuser` - login of your user.

:bangbang: | :Warning: This setting gives user 'xuser' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

set the correct file permissions:

```
chmod 0400 /usr/local/etc/sudoers.d/20_cbsd
```

4) Make sure that the /usr/local/bin/cbsd interpreter is accessible to your user both without sudo and via `sudo` without entering a password:
```
cbsd version
sudo cbsd version
```

If everything is OK, the application is ready for use.

</details>

<details>
  <summary>Linux: AltLinux setup</summary>

> All actions are performed as the privileged user `root` (or use sudo)

1) Add user `cbsd`:
```
useradd cbsd
```
2) Installing dependencies:
```
apt-get install -y sudo bridge-utils edk2-ovmf psmisc make pax rsync sharutils libssh2 libelf libbsd qemu-system-x86 tmux dialog sqlite3 curl libcurl xorriso binutils coreutils nftables
```

3) Obtaining and initializing `CBSD` (CBSD on Linux is experimental and is temporarily distributed as a tarball):
```
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

4) The user you run the application as must be a member of the 'cbsd' group.
```
usermod -a -G cbsd xuser
```
, where `xuser` - login of your user.

Please re-login back in to apply the change.

5) Your user must have permissions to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration.
To do this, create a file */etc/sudoers.d/20_cbsd* with the contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `xuser` - login of your user.

:bangbang: | :Warning: This setting gives user 'xuser' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

Set the correct file permissions:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

6) Make sure that the /usr/local/bin/cbsd interpreter is accessible to your user both without sudo and via `sudo` without entering a password:
```
cbsd version
sudo cbsd version
```

If everything is OK, the application is ready for use.

</details>

<details>
  <summary>Linux: Debian/Ubuntu setup</summary>

> All actions are performed as the privileged user `root` (or use sudo)

1) Add user `cbsd`:
```
useradd cbsd
```
2) Installing dependencies:
```
apt install -y sudo uuid-runtime bridge-utils net-tools ovmf psmisc make pax rsync sharutils libssh2-1 libelf1 libbsd0 qemu-system-x86 tmux dialog sqlite3 curl libcurl4 xorriso nftables coreutils binutils
```

3) Obtaining and initializing `CBSD` (CBSD on Linux is experimental and is temporarily distributed as a tarball):
```
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

4) The user you run the application as must be a member of the 'cbsd' group.
```
usermod -a -G cbsd xuser
```
, where `xuser` - login of your user.

Please re-login back in to apply the change.

5) Your user must have permission to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration.
To do this, create a file */etc/sudoers.d/20_cbsd* with the following contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `xuser` - login of your user.

:bangbang: | :Warning: This setting gives user 'xuser' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

Set the correct file permissions:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

6) Make sure that the /usr/local/bin/cbsd interpreter is accessible to your user both without sudo and via `sudo` without entering a password:
```
cbsd version
sudo cbsd version
```

If everything is OK, the application is ready for use.

</details>

<details>
  <summary>Linux: Manjaro setup</summary>

> All actions are performed as the privileged user `root` (or use sudo)

1) Add user `cbsd`:
```
useradd cbsd
```
2) Installing dependencies:
```
pacman -S sudo bridge-utils bind net-tools ovmf psmisc make pax rsync sharutils libssh2 libelf libbsd qemu-system-x86 tmux dialog sqlite3 curl file xorriso cpio gnu-netcat binutils coreutils nftables
```

3) Obtaining and initializing `CBSD` (CBSD on Linux is experimental and is temporarily distributed as a tarball):
```
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

4) The user you run the application as must be a member of the 'cbsd' group.
```
usermod -a -G cbsd xuser
```
, where `xuser` - login of your user.

Please re-login back in to apply the change.

5) Your user must have permission to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration.
To do this, create a file */etc/sudoers.d/20_cbsd* with the following contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `xuser` - login of your user.

:bangbang: | :Warning: This setting gives user 'xuser' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

Set the correct file permissions:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

6) Make sure that the /usr/local/bin/cbsd interpreter is accessible to your user both without sudo and via `sudo` without entering a password:
```
cbsd version
sudo cbsd version
```

If everything is OK, the application is ready for use.

</details>


# Creating your first virtual machine

### backend selection

If all system settings are met, when starting `mybee-qt`, one of the methods of interaction (Setup Server connection) will be 'File', where Path should be the path to the CBSD interpreter: /usr/local/bin/cbsd.

In addition, you need to select and confirm the SSH key to use. By default, the application uses the key generated by the application itself ( ~/.config/mybee-qt/id_ed25519 ).

![cbsd_local1.png](https://myb.convectix.com/img/cbsd_local1.png?raw=true)

Confirm the key by clicking on this line and proceed to selecting the guest OS profile.

### Guest OS profile selection

In the next step, select the guest OS type (a grouped list of the main derivatives/families (Linux, BSD, Windows, Other)) and the desired distribution itself.
The list of profiles is updated both when updating the application version and by a separate directive. In addition, you can create your own profiles and even gold images based on existing ones - read about this in the [relevant chapter](profiles.md)

![cbsd_local2.png](https://myb.convectix.com/img/cbsd_local2.png?raw=true)

### VM resources: vCPU, RAM, storage

The last step before creation is to select the guest machine configuration in the form of virtual processor cores, RAM, and system disk space.
By default, you are offered the minimum possible characteristics for a particular profile, which guarantee a normal OS startup, but depending on how you will operate the VM,
you may need additional resources. After creation, you can always reconfigure these parameters without recreating the VM.

![cbsd_local3.png](https://myb.convectix.com/img/cbsd_local3.png?raw=true)

### The process of creating a VM

The user no longer takes part in the further process - if the image has not been previously warmed up (use the selected profile for the first time), the application will scan available mirrors for the selected image to obtain the fastest resource for you and will start downloading the image. The speed of this stage depends mainly on the speed of your Internet - just wait until the process of obtaining the image is complete.

---

:information_source: If you are not satisfied with the speed of the mirrors or have difficulty getting an image, [be sure to read this material](https://github.com/cbsd/mirrors).

---


![cbsd_local4.png](https://myb.convectix.com/img/cbsd_local4.png?raw=true)

The image on the basis of which your VM will be created will be saved and cached, so reusing this profile will lead to faster creation of the VM, since there is no need to download the image.
At the end of the VM creation process, the dialog returns you to the familiar main screen of the application.

![cbsd_local5.png](https://myb.convectix.com/img/cbsd_local5.png?raw=true)

To get started with VMs, you can either go to the [Working with VMs](myb-qt-vm.md) chapter, or read about two other methods of working with hypervisors if you have dedicated servers based on [FreeBSD OS](https://www.freebsd.org/)/[Linux](https://kernel.org/) or use the [MyBee](https://myb.convectix.com) distribution.


---

**<< Prev**: [Operating mode N1: local CBSD interpreter](myb-qt-cbsd-local.md)     |     **>> Next**: [Operating mode N2: via SSH protocol, CBSD interpreter/MyBee](myb-qt-cbsd-ssh.md)

