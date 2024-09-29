# Получение и установка MyBee-QT

MyBee-QT является кросс-платформенным ПО. Используйте соответствующей вашей платформе и архитектуре билд:

|           OS name         |  Arch                 |  Link                                                 |
| ------------------------- | --------------------- | ----------------------------------------------------- |
|         Android           |  arm64_little_endian  | link1                                                 |
|         Windows           |  x86_64               | link1                                                 |
|         MacOS             |  x86_64               | link1                                                 |
|         Linux             |  x86_64               | link1                                                 |
|         FreeBSD           |  x86_64               | link1                                                 |


## FreeBSD: сборка из исходных кодов из дерева портов

Вам необходимо иметь дерево портов (обычно он располагается в /usr/ports). Для его получения через git, можно использовать следующую команду:

```
git clone -b main --single-branch '--depth=1' https://git.freebsd.org/ports.git /usr/ports
```


```
env BATCH=no make -C /usr/ports/emulators/mybee-qt install
```

## FreeBSD: установка через `pkg`

Если вы хотите установить пакет, используйте менеджер pkg(8):
```
pkg install -y emulators/mybee-qt
```

Для использования альтернативного 'официального' репозитория с -latest/development версией приложения для FreeBSD/pkg: 


```
mkdir -p /usr/local/etc/pkg/repos
cat > /usr/local/etc/pkg/repos/MyBee-QT.conf <<EOF
MyBee-QT: {
    url: "https://pkg.convectix.com/mybee-qt/\${ABI}/latest",
    mirror_type: "none",
    enabled: yes
}
EOF
```

```
pkg update -f -r MyBee-QT
pkg install -r MyBee-QT mybee-qt
```


# Android


# Linux: установка через AppImage



---

Дальше: [Первоначальная настройка MyBee-QT](setup.md)
