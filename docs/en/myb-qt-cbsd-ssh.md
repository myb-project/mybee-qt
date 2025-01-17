# Mode N2: via SSH protocol, CBSD interpreter/MyBee

This mode allows you to run virtual machines on a dedicated remote server via SSH connection, on which the `cbsd` package is running. At the moment, these are platforms based on Linux and FreeBSD OS. The closest analogue of this mode is virt-manager.

## Preparing the remote host

As a remote hypervisor, you can use OS based on MyBee, Linux, FreeBSD, HardenedBSD, DragonflyBSD, XigmaNAS. It is assumed that the servers are running on bare metal (not virtual machines) - if this is not the case,
please consult the support/configuration guide for nested virtualization before using MyBee-QT.

Use the instructions below, depending on the type of OS/distribution, as there are differences:

<details>
  <summary>Configuring MyBee</summary>

1) Go to the MyBee console (in the browser via http://mybee-srv/shell/ or via SSH) and create a user via item '12)>[ Client SSH user: - ]<', on behalf of which the application will work with the hypervisor. As a rule, a password is not required for such a user in favor of
SSH keys, otherwise you will enter the password for each operation.

![mybqt-ssh1.png](https://myb.convectix.com/img/mybqt-ssh1.png?raw=true)

![mybqt-ssh2.png](https://myb.convectix.com/img/mybqt-ssh2.png?raw=true)

As a result of running the script, you will receive a "one-time" and expiring URL (24 hours), through which the MyBee-QT application can import a key to work with this MyBee instance.

![mybqt-ssh3.png](https://myb.convectix.com/img/mybqt-ssh3.png?raw=true)

:bangbang: | :Attention: the data via this URL allows access to Mybee, so keep the address secret and the sooner you use it - the better. After the first request to this URL or after 24 hours the link expires, so if you accidentally missed the link and you need to re-generate a new URL - use the above point '12'.
:---: | :---

2) Click 'CREATE' -> 'Import credential' in the application and use the URL obtained in step 1. At this point, the preparation for work is complete.

</details>


<details>
  <summary>Configuring FreeBSD</summary>

We are considering the option of setting up vanilla FreeBSD 14.2-RELEASE, immediately after installation.

1) The CBSD package and the corresponding dependencies must be installed on your system:

```
pkg install -y cbsd tmux
```

2) CBSD initialization:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Create a user (for example 'mybee') under which we will work. Don't forget to include it in the `cbsd` group on the question (Invite into other groups):
```
root@f14:~ # adduser
Username: mybee
Full name: mybee
Uid (Leave empty for default):
Login group [mybee]:
Login group is mybee. Invite mybee into other groups? []: cbsd
Login class [default]:
Shell (sh csh tcsh nologin) [sh]:
Home directory [/home/mybee]:
Home directory permissions (Leave empty for default):
Enable ZFS encryption? (yes/no) [no]:
Use password-based authentication? [yes]: no
Lock out the account after creation? [no]:
Username    : mybee
Password    : <disabled>
Full Name   : mybee
Uid         : 1002
ZFS dataset : zroot/home/mybee
Class       : 
Groups      : mybee cbsd
Home        : /home/mybee
Home Mode   : 
Shell       : /bin/sh
Locked      : no
OK? (yes/no) [yes]: yes
adduser: INFO: Successfully created ZFS dataset (zroot/home/mybee).
adduser: INFO: Successfully added (mybee) to the user database.
Add another user? (yes/no) [no]:
Goodbye!
root@f14:~ #
```

If you want to connect from an existing user, add it to the 'cbsd' group separately:
```
pw group mod cbsd -m mybee
```
, where `mybee` is your user login.

4) Your user must have permission to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration.

To do this, create a file */usr/local/etc/sudoers.d/20_mybee* with the following contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `mybee` is your user's login.

:bangbang: | :Warning: This setting gives the user 'sshuser' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

set the correct file permissions:

```
chmod 0400 /usr/local/etc/sudoers.d/20_mybee
```

</details>

<details>
  <summary>Configuring Linux: AltLinux</summary>

> All actions are performed as the privileged user `root` (or use sudo)

1) Installing CBSD and dependencies. Obtaining and initializing `CBSD` (CBSD on Linux is experimental and is temporarily distributed as a tarball):
```
apt-get install -y sudo bridge-utils edk2-ovmf psmisc make pax rsync sharutils libssh2 libelf libbsd qemu-system-x86 tmux dialog sqlite3 curl libcurl xorriso binutils coreutils nftables
```
```
useradd cbsd
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
```

2) CBSD initialization:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Create a user (for example 'mybee') under which we will work. Don't forget to include it in the `cbsd` group:

```
useradd cbsd
usermod -a -G cbsd mybee
```
, where `mybee` is your user's login.

4) Your user must have permission to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration. To do this, create a file */etc/sudoers.d/20_mybee* with the following contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `mybee` is your user's login.

:bangbang: | :Warning: This setting gives user 'mybee' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

Set the correct file permissions:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

</details>

<details>
  <summary>Configuring Linux: Debian/Ubuntu</summary>

> All actions are performed as the privileged user `root` (or use sudo)

1) Installing CBSD and dependencies. Obtaining and initializing `CBSD` (CBSD on Linux is experimental and is temporarily distributed as a tarball):
```
apt install -y sudo uuid-runtime bridge-utils net-tools ovmf psmisc make pax rsync sharutils libssh2-1 libelf1 libbsd0 qemu-system-x86 tmux dialog sqlite3 curl libcurl4 xorriso nftables coreutils binutils
```
```
useradd cbsd
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
```

2) CBSD initialization:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Create a user (for example 'mybee') under which we will work. Don't forget to include it in the `cbsd` group:

```
useradd cbsd
usermod -a -G cbsd mybee
```
, where `mybee` is your user's login.

4) Your user must have permission to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration. To do this, create a file */etc/sudoers.d/20_mybee* with the following contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `mybee` is your user's login.

:bangbang: | :Warning: This setting gives user 'mybee' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

Set the correct file permissions:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

</details>

<details>
  <summary>Configuring Linux: Manjaro</summary>


> All actions are performed as the privileged user `root` (or use sudo)

1) Installing CBSD and dependencies. Obtaining and initializing `CBSD` (CBSD on Linux is experimental and is temporarily distributed as a tarball):
```
pacman -S sudo bridge-utils bind net-tools ovmf psmisc make pax rsync sharutils libssh2 libelf libbsd qemu-system-x86 tmux dialog sqlite3 curl file xorriso cpio gnu-netcat binutils coreutils nftables
```
```
useradd cbsd
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
```

2) CBSD initialization:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Create a user (for example 'mybee') under which we will work. Don't forget to include it in the `cbsd` group:

```
useradd cbsd
usermod -a -G cbsd mybee
```
, where `mybee` is your user's login.

4) Your user must have permission to run the /usr/local/bin/cbsd interpreter as the 'root' user via sudo configuration. To do this, create a file */etc/sudoers.d/20_mybee* with the following contents:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, where `mybee` is your user's login.

:bangbang: | :Warning: This setting gives user 'mybee' 'root' privileges on the host system via the /usr/local/bin interpreter!
:---: | :---

Set the correct file permissions:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

</details>

## Private and public access key

5) Generate private and public SSH keys for user `mybee`:
```
su - mybee -c "ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519 -N ''"
```

and copy the .pub key to authorized_keys:
```
cp -a ~mybee/.ssh/id_ed25519.pub ~mybee/.ssh/authorized_keys
```

6) Take the private `~mybee/.ssh/id_ed25519` and public `~mybee/.ssh/id_ed25519.pub` key to the client host where we plan to use MyBee-QT.
Let's save these files, for example, in the ~/private/ directory under the name f14.my.domain-mybee.key (private) and f14.my.domain-mybee.key.pub (public, the file name must match the name of the private file + the .pub extension is required), so that we know from which server these keys are ('f14.my.domain' in our example corresponds to the FQDN of the remote server, and 'mybee' is the login on this server), not forgetting to set secure access rights:
```
mkdir ~/private
chmod 0700 ~/private
chmod 0600 ~/private/f14.my.domain-mybee.key
```

7) Make sure you are connecting via SSH as user `mybee` using a private key and the /usr/local/bin/cbsd interpreter is accessible without entering a password:
```
ssh -i ~/private/f14.my.domain-mybee.key mybee@<host> cbsd version
ssh -i ~/private/f14.my.domain-mybee.key mybee@<host> sudo cbsd version
```

If everything is OK, the application is ready for use.


# Creating your first virtual machine

## backend selection

In MyBee-QT, click the [+] Create button and in the dialog box, use the following elements to configure the connection:

- 1) add and import the private key of the remote host;
- 2) the key should appear as an available one for selection;
- 3) specify the parameters for connecting to the remote host (in our example, this is 172.16.0.91);
- 4) the method of working with the hypervisor - via SSH;

![mybqt-ssh5.png](https://myb.convectix.com/img/mybqt-ssh5a.png?raw=true)

dialog for selecting a private key from the file system (select .key, but there should be a similar file with the .pub extension nearby):

![mybqt-ssh4.png](https://myb.convectix.com/img/mybqt-ssh4.png?raw=true)

When you click the 'Profile' button, you are taken to the selection of profiles available on the remote server. If you are connecting for the first time, you will see an information window with the key hash:

![mybqt-ssh6.png](https://myb.convectix.com/img/mybqt-ssh6.png?raw=true)

If everything is OK, then on the next screen you will see a selection of available engines on the remote system that you can use:

![mybqt-ssh7.png](https://myb.convectix.com/img/mybqt-ssh7.png?raw=true)

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

**<<_**__[Next: Mode N1: local CBSD interpreter](myb-qt-cbsd-local.md)__ $~~~$ | $~~~$ __[Next: Mode N3: via RestAPI](myb-qt-cbsd-api.md)__**_>>**
