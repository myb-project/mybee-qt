# Получение и установка MyBee-QT

MyBee-QT является cross-platform ПО. Используйте соответствующей вашей платформе и архитектуре билд:

|           OS name         |  Arch                 |  Link                                                 |
| ------------------------- | --------------------- | ----------------------------------------------------- |
|         Android           |  arm64_little_endian  | link1                                                 |
|         Windows           |  x86_64               | link1                                                 |
|         MacOS             |  x86_64               | link1                                                 |
|         Linux             |  x86_64               | link1                                                 |
|         FreeBSD           |  x86_64               | link1                                                 |



# Android

WIP..

# Windows

WIP..

# MacOS

WIP..

# Linux

WIP..

# FreeBSD

### из исходных кодов, дерево портов

Установка из исходных кодов и дерева портов FreeBSD:

```
env BATCH=no make -C /usr/ports/emulators/mybee-qt install
```

### из пакета, репозиторий FreeBSD


Установка пакета из официального репозитория FreeBSD:
```
pkg install -y mybee-qt
```

### из пакета, репозиторий MyBee-QT-development


Установка пакета (latest/unstable/testing) из pkg репозитория MyBee-QT:

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


---

Дальше: [Режим работы N1: локальный CBSD интерпретатор](myb-qt-cbsd-local.md)
