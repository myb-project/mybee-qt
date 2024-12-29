# Режим работы N1: локальный, через CBSD интерпретатор

Данный режим позволяет запускать виртуальные машины локально на тех платформах, на которых установлено программное обеспечение [CBSD](https://github.com/cbsd/cbsd). В данный момент, это платформы на базе Linux, FreeBSD, XigmaNAS, DragonflyBSD и TrueNAS CORE.

Наиболее близкий аналог данного режима работы - Oracle VirtualBox.

Если Ваша рабочая система - мобильное устройство, MacOS или Windows, вам подойдут два других способа ( [Режим работы N2: через протокол SSH, CBSD интерпретатор/MyBee](myb-qt-cbsd-ssh.md) или [Режим работы N3: через RestAPI/MyBee](myb-qt-api.md) ).

В данном режиме, MyBee-QT взаимодействует напрямую с интерпретатором /usr/local/bin/cbsd, поэтому вам потребуется предварительно выполнить установку дополнительного ПО и настройку прав доступа.

<details>
  <summary>Настройка FreeBSD</summary>

1) В вашей системе должен быть установлен пакет CBSD и соответствующие зависимости:

```
pkg install -y cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

2) Ининциализация CBSD:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Пользователь, от которого вы запускаете приложение, должен входить в группу 'cbsd'.
```
pw group mod cbsd -m xuser
```
, где `xuser` - login вашего пользователя.

Перезайдите в систему, чтобы применить изменение.

3) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo.

Для этого, создайте файл */usr/local/etc/sudoers.d/20_cbsd* со следующим содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `xuser` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'xuser' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

установите корректные права доступа на файл:

```
chmod 0400 /usr/local/etc/sudoers.d/20_cbsd
```

4) Убедитесь, что интерпретатор /usr/local/bin/cbsd доступен для вашего пользователя как без sudo, так и через `sudo` без ввода пароля:
```
cbsd version
sudo cbsd version
```

Если все в порядке, приложение готово к использованию.

</details>

<details>
  <summary>Настройка Linux: AltLinux</summary>

> Все действия выполняются от привелигированного пользователя `root` (либо используйте sudo) 

1) Добавьте пользователя `cbsd`:
```
useradd cbsd
```
2) Установка зависимостей:
```
apt-get install -y sudo bridge-utils edk2-ovmf psmisc make pax rsync sharutils libssh2 libelf libbsd qemu-system-x86 tmux dialog sqlite3 curl libcurl xorriso binutils coreutils nftables
```

3) Получение и инициализация `CBSD` (CBSD на Linux носит эксперементальный характер и временно распространяется в виде тарбола):
```
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

4) Пользователь, от которого вы запускаете приложение, должен входить в группу 'cbsd'.
```
usermod -a -G cbsd xuser
```
, где `xuser` - login вашего пользователя.

Перезайдите в систему, чтобы применить изменение.

5) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo.
Для этого, создайте файл */etc/sudoers.d/20_cbsd* с содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `xuser` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'xuser' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

Установите корректные права доступа на файл:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

6) убедитесь, что интерпретатор /usr/local/bin/cbsd доступен для вашего пользователя как без sudo, так и через `sudo` без ввода пароля:
```
cbsd version
sudo cbsd version
```

Если все в порядке, приложение готово к использованию.

</details>

<details>
  <summary>Настройка Linux: Debian/Ubuntu</summary>

> Все действия выполняются от привелигированного пользователя `root` (либо используйте sudo) 

1) Добавьте пользователя `cbsd`:
```
useradd cbsd
```
2) Установка зависимостей:
```
apt install -y sudo uuid-runtime bridge-utils net-tools ovmf psmisc make pax rsync sharutils libssh2-1 libelf1 libbsd0 qemu-system-x86 tmux dialog sqlite3 curl libcurl4 xorriso nftables coreutils binutils
```

3) Получение и инициализация `CBSD` (CBSD на Linux носит эксперементальный характер и временно распространяется в виде тарбола):
```
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

4) Пользователь, от которого вы запускаете приложение, должен входить в группу 'cbsd'.
```
usermod -a -G cbsd xuser
```
, где `xuser` - login вашего пользователя.

Перезайдите в систему, чтобы применить изменение.

5) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo.
Для этого, создайте файл */etc/sudoers.d/20_cbsd* с содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `xuser` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'xuser' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

Установите корректные права доступа на файл:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

6) убедитесь, что интерпретатор /usr/local/bin/cbsd доступен для вашего пользователя как без sudo, так и через `sudo` без ввода пароля:
```
cbsd version
sudo cbsd version
```

Если все в порядке, приложение готово к использованию.

</details>

<details>
  <summary>Настройка Linux: Manjaro</summary>

> Все действия выполняются от привелигированного пользователя `root` (либо используйте sudo) 

1) Добавьте пользователя `cbsd`:
```
useradd cbsd
```
2) Установка зависимостей:
```
apt-get install -y sudo bridge-utils edk2-ovmf psmisc make pax rsync sharutils libssh2 libelf libbsd qemu-system-x86 tmux dialog sqlite3 curl libcurl xorriso binutils coreutils nftables
```

3) Получение и инициализация `CBSD` (CBSD на Linux носит эксперементальный характер и временно распространяется в виде тарбола):
```
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
rm -f cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/cbsd
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

4) Пользователь, от которого вы запускаете приложение, должен входить в группу 'cbsd'.
```
usermod -a -G cbsd xuser
```
, где `xuser` - login вашего пользователя.

Перезайдите в систему, чтобы применить изменение.

5) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo.
Для этого, создайте файл */etc/sudoers.d/20_cbsd* с содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
xuser        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `xuser` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'xuser' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

Установите корректные права доступа на файл:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

6) убедитесь, что интерпретатор /usr/local/bin/cbsd доступен для вашего пользователя как без sudo, так и через `sudo` без ввода пароля:
```
cbsd version
sudo cbsd version
```

Если все в порядке, приложение готово к использованию.

</details>


# Создание первой виртуальной машины

### выбор бакенда

Если все настройки системы соблюдены, при запуске `mybee-qt`, в качестве одного из методов взаимодействия (Setup Server connection) будет 'File', где в качестве Path должен быть указан путь до интерпретатора CBSD: /usr/local/bin/cbsd.

Кроме этого, вам необходимо выбрать и подтвердить используемый SSH ключ, по-умолчанию, приложение будет использовать системный локальный ключ CBSD ( /usr/jails/.ssh/id_rsa ).

![cbsd_local1.png](/docs/images/cbsd_local1.png)

Подтвердите ключ кликнув на эту строчку и переходите к выбору профиля гостевой ОС.

### выбор профиля гостевой ОС

На следующем шаге, выберете тип гостевой ОС (группированный список из основных деривативов/семейств (Linux, BSD, Windows, Other)) и непосредственно желаемый дистрибутив.
Список профилей обновляется как при обновлении версии приложения, так и отдельной директивой. Кроме этого, вы можете создавать свои собственные профили и даже gold-образы на базе уже существующих - об этом, читайте в [соответствующей главе](profiles.md)

![cbsd_local2.png](/docs/images/cbsd_local2.png)

### ресурсы ВМ: vCPU, RAM, storage

Последний шаг перед созданием - выбор конфигурации гостевой машины в виде характеристик ядер виртуального процессора, объем оперативной памяти и дисковое пространство системного диска.
По-умолчанию, вам предлагаются минимально возможные для того или иного профиля характеристики, которые гарантируют нормльный запуск ОС, однако в зависимости от того, как вы будете эксплуатировать ВМ,
вам могут потребоваться дополнительные ресурсы. После создания, вы всегда можете реконфигурировать эти параметры без пересоздания ВМ.

![cbsd_local3.png](/docs/images/cbsd_local3.png)

### Процесс создания ВМ

В дальнейшем процессе польователь уже не принимает участие  - если образ не был ранее прогрет (используете выбранный профиль первый раз), приложение просканирует доступные зеркала для выбранного образа для получения максимально быстрого для
вас ресурса и начнет скачивать образ. Скорость работы данного этапа зависит, в основном, от скорости вашего интернета - просто ожидайте, когда процесс получения образа завершниться.

![cbsd_local4.png](/docs/images/cbsd_local4.png)

Образ, на основе которого будет создана ваша ВМ будет сохранена и закеширована, поэтому повтороное использование данного профиля приведет к более быстрому созданию ВМ, поскольку скачивать образ уже не потребуется.
По окончанию процесса создании ВМ, диалог возвращает вас в привычный основной экран приложения.

![cbsd_local5.png](/docs/images/cbsd_local5.png)


Чтобы начать работу в ВМ, можете перейти к главе [Работа с ВМ](myb-qt-vm.md), либо прочесть о двух других методах работы с гипервизорами, если у вас есть в наличии выделенные сервера на базе [FreeBSD OS](https://www.freebsd.org/)/[Linux](https://kernel.org/) или используете дистрибутив [MyBee](https://myb.convectix.com).


---

Дальше: [Режим работы N2: через протокол SSH, CBSD интерпретатор/MyBee](myb-qt-cbsd-ssh.md)
