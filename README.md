# MyBee-QT: cross-platform GUI client for CBSD

<p align="center">
  <span>English</span> |
  <a href="/README.es.md">Español</a> |
  <a href="/README.fr.md">Français</a> |
  <a href="/README.de.md">Deutsch</a> |
  <a href="/README.ru.md">Русский</a> |
  <a href="/README.ch.md">中文</a> |
</p>

---

:information_source: This guide covers the administration and use of [MyBee-QT](https://github.com/myb-project/mybee-qt), a graphical client for [CBSD](https://github.com/cbsd/cbsd) (and [MyBee OS](https://myb.convectix.com/)).

---

## Welcome to MyBee-QT

(*) What is **MyBee-QT**: it is a cross-platform QT-based application that provides a graphical interface for working with virtual machines and containers (creation, deletion, start and stop) on those platforms where **CBSD** can work.

It supports three modes:

1) locally (similar to the virsh or VirtualBox mode): you install it on your Desktop computer running Linux or BSD and work locally with virtual environments. In this case, MyBee-QT works directly with the `cbsd` interpreter via `shell`;

2) remotely via a simple RestAPI service: you use your Desktop computer (it can be a mobile device, such as a phone) as a thin client and work with a Linux or FreeBSD server on which **CBSD** and RestAPI to it are installed.

3) remotely via SSH with the `cbsd` interpreter via `shell` (similar to how virt-manager works): the method is close to point 1, but SSH is added as a transport. This method does not require additional installation of the RestAPI service and turns any server running Linux/FreeBSD and OpenSSH into a host for your virtual environments.

![mybee-qt-overview2.png](https://myb.convectix.com/img/mybee-qt-overview2.png?raw=true)

You are not limited to using any one method - one copy of **MyBee-QT** can serve both local environments and RestAPI/OpenSSH-only systems at the same time.

(*) What is **CBSD**: it is a cross-platform (FreeBSD, Linux, DragonFlyBSD) library (set of scripts) for low-level work with virtual environments (creation, deletion, launch and shutdown) working with hypervisors such as: bhyve, KVM, QEMU, XEN, VirtualBox, as well as lightweight containers based on jail technology.
By installing the **CBSD** package on any Linux or FreeBSD node, you get the ability to use hypervisors and node resources for virtual machines, creating them in the GUI.
The **MyBee-QT** application owes its appearance to the **CBSD** project and its community, since the development was sponsored by the [CBSD donation fund](https://www.patreon.com/clonos).

(*) What is **MyBee OS**: this is an experimental distribution (two variants - based on Linux/Debian and FreeBSD) with pre-installed **CBSD**, RestAPI and OpenSSH services, optimized for creating virtual environments.
It was created as a PoC, demonstrating that having bare metal without any OS, within 5 minutes you can already start working in virtual machines using server resources (~1-2 minutes takes the firmware of the MyBee hypervisor, then everything depends on the speed of your Internet connection).
However, in addition to the demo nature, the distribution can find applications in trusted environments.

With **MyBee-QT**, the user can get a hybrid cloud consisting of various Linux, FreeBSD, DragonFlyBSD (the list will be expanded) systems that run VMs simultaneously on different hypervisors: bhyve, XEN, KVM, QEMU, NVMM, VirtualBox (the list will be expanded);

The **MyBee-QT** project focuses on maximum simplicity for the user, for this reason, a huge number of **CBSD** features and complex configuration cases are ignored. If you are missing any features:

- review the roadmap, perhaps it is planned in the very near future;
- file an Issue/Feature request - perhaps it should be added to the roadmap if it is in demand by the majority;
- consider using **CBSD** (maximum features) directly or use the WEB UI to CBSD in the form of [ClonOS](https://clonos.convectix.com);

### GUI

![mybee-qt-overview.png](https://myb.convectix.com/img/mybee-qt-overview.png?raw=true)

The application view can work in two modes - 'DesktopPC' mode (on the screenshot) and mobile device mode (compact). The main screen contains the following elements:

1) The main menu of application settings'
2) The list of hosters that you manage and use resources to create virtual environments. The application on the screenshot uses three nodes - 'rio' - a server running via RestAPI (can be running FreeBSD/XigmaNAS/DragonFlyBSD/Linux/MyBee OS),
'bravo' - running via SSH (can be running FreeBSD/XigmaNAS/DragonFlyBSD/Linux/MyBee OS) and finally, a local computer via the `cbsd` interpreter.
3) virtual environments. These can be VMs running bhyve, KVM, QEMU, XEN, NVMM, VirtualBox hypervisors or containers running Jail, SystemD-nspawn, Docker/Containerd..
4) Button to launch the wizard for creating a new virtual environment;
5) Controls for the selected virtual environment (also available via the context menu on the environment name);
6) Text console or SSH terminal directly into the virtual environment - available if the target environment supports console login;
7) Graphical console directly into the virtual environment - available if the target environment supports VNC or RDP access (windows, ubuntu-desktop, kali, ghostbsd, etc.). Some containers may also offer graphical access.

![mybee-graph-jail.png](https://myb.convectix.com/img/mybee-graph-jail.png)

For example, here is a demonstration of launching and displaying the game [https://github.com/TheAssemblyArmada/Vanilla-Conquer](Vanilla-Conquerer (original Command&Conqueror)) from a jail container on the FreeBSD operating system.
By pressing the 'F11' button, you can switch the graphical console to full-screen mode and thus get something similar to VDI. If you allow the container or virtual machine to use the GPU, which will greatly increase the performance of the graphics system, you can distribute/share and work in graphical containers and virtual machines with comfort.

8) Context menu with a set of auxiliary operations, such as setting filters for a selective list of virtual environments.
9) The application workspace, at different times can show different information - graphs/telemetry of system load, graphical or text console, just debug/log of the application, etc.

## System Requirements

- Android mobile or Linux or FreeBSD desktop environment;
- 1 cpu, 256 MB RAM;
- ~64 MB storage;
- access to CBSD infrastructure mirrors to obtain gold/ISO images ( optional [if you set up your own cache/mirror](https://github.com/cbsd/mirrors) );

## Roadmap

Planned features in the next version:

- reconfiguration of virtual environment characteristics;
- display of graphs/telemetry of virtual machines and hosts (CPU, Memory, Storage/Network I/O);
- IOS, MacOS, Windows port (help from porters or donors (including hardware) is welcome);
- getting rid of 'root' privileges on those hypervisors where possible;
- integration with [Polkit](https://github.com/polkit-org/polkit);
- migration and cloning of virtual environments between hypervisors/nodes;

## MyBee-QT Handbook

* [Getting, installation and initial setup](docs/en/get-myb-qt.md)
* [Operating mode N1: local, CBSD interpreter](docs/en/myb-qt-cbsd-local.md)
* [Operating mode N2: via SSH protocol, CBSD interpreter/MyBee](docs/en/myb-qt-cbsd-ssh.md)
* [Operating mode N3: via RestAPI/MyBee/XigmaNAS](docs/en/myb-qt-api.md)

