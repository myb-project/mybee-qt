# Получение и установка MyBee-QT

MyBee-QT является cross-platform ПО. Используйте соответствующей вашей платформе и архитектуре билд (по ссылкам опубликован всегда последняя актуальная версия):

|           OS name         |  Arch                 |  Link                                                                            |
| ------------------------- | --------------------- | -------------------------------------------------------------------------------- |
|         Android           |  arm64_little_endian  | [mybee-qt.apk](https://myb.convectix.com/DL/apk/mybee-qt.apk)                    |
|         Windows           |  x86_64               | WIP, Porters or hardware donors help is welcome                                  |
|         MacOS             |  x86_64               | WIP, Porters or hardware donors help is welcome                                  |
|         Linux             |  x86_64               | [mybee-qt](https://myb.convectix.com/DL/linux/mybee-qt)                          |
|         FreeBSD           |  x86_64               | `pkg install -y mybee-qt` [^#1]                                                  |

<details>
  <summary>Linux-based системы</summary>

## Linux 

Официальный билд MyBee-QT под Linux платформу представляет из себя [AppImage](https://appimage.org/) приложение, соответственно, убедитесь, что ваша система имеет установленным `libfuse` пакет и модуль `fuse` загружен:

```
modprobe fuse
```

Получите билд и убедитесь, что он выполняемый:
```
wget https://myb.convectix.com/DL/linux/mybee-qt
chmod +x ./mybee-qt
./mybee-qt
```

</details>

<details>
  <summary>FreeBSD OS</summary>

## FreeBSD

Официально поддерживатся несколько методов получения приложения.

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

</details>

Дальнейшая настройка зависит от методов взаимодействия с CBSD (см. ниже).

---

**<<_**__[Назад: Добро пожаловать в MyBee-QT](myb-qt-cbsd-local.md)__ $~~~$ | $~~~$ __[Дальше: Режим работы N1: локальный CBSD интерпретатор](myb-qt-cbsd-local.md)__**_>>**


[^#1]: В отличии от Linux, на FreeBSD нет необходимости прибегать в различным AppImage/Flatpak/Snap и прочим форматам, посколько кластера FreeBSD собирают пакеты
одним "срезом" портов, таким образом гарантируя, что билды успешно собираются и линкованы с совместимыми для сборки версиями.
