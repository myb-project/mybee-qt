# Getting, installation and initial setup

MyBee-QT is a cross-platform software. Use the appropriate platform and architectural project for you (the links always publish the latest current version):

|           OS name         |  Arch                 |  Link                                                                            |
| ------------------------- | --------------------- | -------------------------------------------------------------------------------- |
|         Android           |  arm64_little_endian  | [mybee-qt.apk](https://myb.convectix.com/DL/apk/mybee-qt.apk)                    |
|         Windows           |  x86_64               | WIP, Porters or hardware donors help is welcome                                  |
|         MacOS             |  x86_64               | WIP, Porters or hardware donors help is welcome                                  |
|         Linux             |  x86_64               | [mybee-qt](https://myb.convectix.com/DL/linux/mybee-qt)                          |
|         FreeBSD           |  x86_64               | `pkg install -y mybee-qt` [^#1]                                                  |

## Linux 

The official build of MyBee-QT for Linux platform is [AppImage](https://appimage.org/) application, so make sure your system has `libfuse` package installed and `fuse` module loaded:

```
modprobe fuse
```

Get the build and make sure it runs:
```
wget https://myb.convectix.com/DL/linux/mybee-qt
chmod +x ./mybee-qt
./mybee-qt
```

Further configuration depends on the methods of interaction with CBSD (see below).

## FreeBSD

There are several officially supported methods for obtaining the application.

### from source code, ports tree

Installation from FreeBSD source and ports tree:

```
env BATCH=no make -C /usr/ports/emulators/mybee-qt install
```

### from package, FreeBSD repository

Installing a package from the official FreeBSD repository:
```
pkg install -y mybee-qt
```

### from package, repository MyBee-QT-development

Installing the package (latest/unstable/testing) from the MyBee-QT pkg repository:

```
cat > /usr/local/etc/pkg/repos/MyBee-QT.conf <<EOF
MyBee-QT: {
    url: "https://pkg.convectix.com/mybee-qt/\${ABI}/latest",
    mirror_type: "none",
    enabled: yes
}
EOF

pkg update -r MyBee-QT
pkg install -r MyBee-QT -y mybee-qt

```

Further configuration depends on the methods of interaction with CBSD (see below).

---

Next: [Operating mode N1: local CBSD interpreter](myb-qt-cbsd-local.md)

[^#1]: Unlike Linux, on FreeBSD there is no need to resort to various AppImage/Flatpak/Snap and other formats, since FreeBSD clusters build packages with a single "slice" of ports, thus ensuring that builds are successfully built and linked with build-compatible versions.
