# Режим работы N2: через протокол SSH, CBSD интерпретатор/MyBee

Данный режим позволяет запускать через SSH соединение виртуальные машины на выделенном удаленном сервере, на которых работает пакет `cbsd`. В данный момент это платформы на базе Linux и FreeBSD OS.
Наиболее близкий аналог работы данного режима - virt-manager.

## Подготовка удаленного хоста

В качестве удаленного гипервизора, вы можете использовать ОС на базе MyBee, Linux, FreeBSD, HardenedBSD, DragonflyBSD, XigmaNAS. Предполагается, что сервера работают на чистом железе (не виртуальные машины) - если это не так,
обратитесь в руководству по поддержке/настройки вложенной виртуализации, прежде чем использовать MyBee-QT.

Используйте инструкцию ниже, в зависимости от типа ОС/дистрибутива, поскольку имеются отличия:

<details>
  <summary>Настройка MyBee</summary>

1) Зайдите в консоль MyBee ( в броузере через http://mybee-srv/shell/ либо через SSH ) и создайте пользователя через пункт '12)>[ Client SSH user: - ]<', от имени которого приложение будет работать с гипервизором. Пароль, как правило, использовать для такого пользователя не требуется в пользу
SSH ключей, иначе вы будете вводить пароль при каждой операции.

![mybqt-ssh1.png](https://myb.convectix.com/img/mybqt-ssh1.png?raw=true)

![mybqt-ssh2.png](https://myb.convectix.com/img/mybqt-ssh2.png?raw=true)

В результате работы скрипта вы получите "одноразовый" и устаревающий URL (24 часа), через который приложение MyBee-QT может импортировать ключ для работы с этим MyBee инстансом.

![mybqt-ssh3.png](https://myb.convectix.com/img/mybqt-ssh3.png?raw=true)

:bangbang: | :Внимание: данные через этот URL позволяют получить доступ к Mybee, поэтому храните адрес в секрете и чем быстрее вы его используете - тем лучше. После первого же обращения на этот URL или через 24 часа ссылка устаревает, поэтому если вы случайно пропустили ссылку и вам нужно повторить генерацию нового URL - используйте вышеупомянутый пункт '12'.
:---: | :---

2) Нажмите в приложении 'CREATE' -> 'Import credential' и используйте URL, полученный в шаге 1. На данном этапе подготовка к работе завершена.

</details>


<details>
  <summary>Настройка FreeBSD</summary>

Рассматриваем вариант настройки ванильной FreeBSD 14.2-RELEASE, сразу после установки.

1) В вашей системе должен быть установлен пакет CBSD и соответствующие зависимости:

```
pkg install -y cbsd tmux
```

2) Ининциализация CBSD:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Создать пользователя (например 'mybee'), от которого будем работать. Не забудем включить его в группу `cbsd` на вопросе (Invite into other groups):
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

Если вы хотите подключаться уже от существующего пользователя, добавьте его в группу 'cbsd' отдельно:
```
pw group mod cbsd -m mybee
```
, где `mybee` - login вашего пользователя.

4) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo.

Для этого, создайте файл */usr/local/etc/sudoers.d/20_mybee* со следующим содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `mybee` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'sshuser' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

установите корректные права доступа на файл:

```
chmod 0400 /usr/local/etc/sudoers.d/20_mybee
```

</details>

<details>
  <summary>Настройка Linux: AltLinux</summary>

> Все действия выполняются от привелигированного пользователя `root` (либо используйте sudo)

1) Установка CBSD и зависимостей. Получение и инициализация `CBSD` (CBSD на Linux носит эксперементальный характер и временно распространяется в виде тарбола):
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

2) Ининциализация CBSD:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Создать пользователя (например 'mybee'), от которого будем работать. Не забудем включить его в группу `cbsd`:

```
useradd cbsd
usermod -a -G cbsd mybee
```
, где `mybee` - login вашего пользователя.

4) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo. Для этого, создайте файл */etc/sudoers.d/20_mybee* с содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `mybee` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'mybee' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

Установите корректные права доступа на файл:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

</details>

<details>
  <summary>Настройка Linux: Debian/Ubuntu</summary>

> Все действия выполняются от привелигированного пользователя `root` (либо используйте sudo)

1) Установка CBSD и зависимостей. Получение и инициализация `CBSD` (CBSD на Linux носит эксперементальный характер и временно распространяется в виде тарбола):
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

2) Ининциализация CBSD:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Создать пользователя (например 'mybee'), от которого будем работать. Не забудем включить его в группу `cbsd`:

```
useradd cbsd
usermod -a -G cbsd mybee
```
, где `mybee` - login вашего пользователя.

4) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo. Для этого, создайте файл */etc/sudoers.d/20_mybee* с содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `mybee` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'mybee' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

Установите корректные права доступа на файл:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

</details>

<details>
  <summary>Настройка Linux: Manjaro</summary>

> Все действия выполняются от привелигированного пользователя `root` (либо используйте sudo)

1) Установка CBSD и зависимостей. Получение и инициализация `CBSD` (CBSD на Linux носит эксперементальный характер и временно распространяется в виде тарбола):
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

2) Ининциализация CBSD:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Создать пользователя (например 'mybee'), от которого будем работать. Не забудем включить его в группу `cbsd`:

```
useradd cbsd
usermod -a -G cbsd mybee
```
, где `mybee` - login вашего пользователя.

4) У вашего пользователя должны быть полномочия на запуск интерпретатора /usr/local/bin/cbsd от пользователя 'root' через конфигурацию sudo. Для этого, создайте файл */etc/sudoers.d/20_mybee* с содержимым:
```
Defaults     env_keep += "workdir DIALOG NOCOLOR CBSD_RNODE"
Cmnd_Alias   MYB_CBSD_CMD = /usr/local/bin/cbsd
mybee        ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `mybee` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'mybee' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

Установите корректные права доступа на файл:

```
chmod 0400 /etc/sudoers.d/20_cbsd
```

</details>

## Приватный и публичный ключ доступа

5) Сгенерируйте приватный и публичный SSH ключ для пользователя `mybee`:
```
su - mybee -c "ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519 -N ''"
```

и скопируйте .pub ключ в authorized_keys:
```
cp -a ~mybee/.ssh/id_ed25519.pub ~mybee/.ssh/authorized_keys
```

6) Забираем приватный `~mybee/.ssh/id_ed25519` и публичный `~mybee/.ssh/id_ed25519.pub` ключ на клиентский хост, где планируем использовать MyBee-QT.
Сохраним эти файлы, например, в каталог ~/private/ под именем f14.my.domain-mybee.key (приватный) и f14.my.domain-mybee.key.pub (публичный, имя файла должно совпадать с именем приватного файла + расширение .pub - обязательно), чтобы знать от какого сервера эти ключи ('f14.my.domain' в нашем примере соответствует FQDN удаленного сервера, а 'mybee' - login на этом сервере), не забыв установить безопасные права доступа:
```
mkdir ~/private
chmod 0700 ~/private
chmod 0600 ~/private/f14.my.domain-mybee.key
```

7) Убедитесь, что вы соединяетесь по SSH от пользователя `mybee` с использованием приватного ключа и интерпретатор /usr/local/bin/cbsd доступен без ввода пароля:
```
ssh -i ~/private/f14.my.domain-mybee.key mybee@<host> cbsd version
ssh -i ~/private/f14.my.domain-mybee.key mybee@<host> sudo cbsd version
```

Если все в порядке, приложение готово к использованию.


# Создание первой виртуальной машины

## выбор бакенда

В MyBee-QT нажмите кнопку [+] Create и в диалоговом окне используйте следующие элементы для конфигурирования подключения:

  - 1) добавьте и импортируйте приватный ключ удаленного хоста;
  - 2) ключ должен появиться в качестве доступных для выбора;
  - 3) укажите параметры подключения к удаленному хосту (в нашем примере это 172.16.0.91);
  - 4) метод работы с гипервизором - через SSH;

![mybqt-ssh5.png](https://myb.convectix.com/img/mybqt-ssh5a.png?raw=true)

диалог выбора приватного ключа с файловой системы (выбираем .key, но рядом должен лежать аналогичный файл с расширением .pub):

![mybqt-ssh4.png](https://myb.convectix.com/img/mybqt-ssh4.png?raw=true)

при нажатии кнопки 'Profile' вы переходите к выбору профилей, доступных на удаленном сервере. Если вы подключаетесь первый раз, вы увидете окно информационного характера с хешем ключа:

![mybqt-ssh6.png](https://myb.convectix.com/img/mybqt-ssh6.png?raw=true)

Если все в порядке, то на следующем экране вы увидите выбор доступных движков на удаленной системе, которые вы можете использовать:

![mybqt-ssh7.png](https://myb.convectix.com/img/mybqt-ssh7.png?raw=true)


## выбор профиля гостевой ОС

На следующем шаге, выберете тип гостевой ОС (группированный список из основных деривативов/семейств (Linux, BSD, Windows, Other)) и непосредственно желаемый дистрибутив.
Список профилей обновляется как при обновлении версии приложения, так и отдельной директивой. Кроме этого, вы можете создавать свои собственные профили и даже gold-образы на базе уже существующих - об этом, читайте в [соответствующей главе](profiles.md)

![cbsd_local2.png](https://myb.convectix.com/img/cbsd_local2.png?raw=true)

## ресурсы ВМ: vCPU, RAM, storage

Последний шаг перед созданием - выбор конфигурации гостевой машины в виде характеристик ядер виртуального процессора, объем оперативной памяти и дисковое пространство системного диска.
По-умолчанию, вам предлагаются минимально возможные для того или иного профиля характеристики, которые гарантируют нормльный запуск ОС, однако в зависимости от того, как вы будете эксплуатировать ВМ,
вам могут потребоваться дополнительные ресурсы. После создания, вы всегда можете реконфигурировать эти параметры без пересоздания ВМ.

![cbsd_local3.png](https://myb.convectix.com/img/cbsd_local3.png?raw=true)

## Процесс создания ВМ

В дальнейшем процессе пользователь уже не принимает участие  - если образ не был ранее прогрет (используете выбранный профиль первый раз), приложение просканирует доступные зеркала для выбранного образа для получения максимально быстрого для
вас ресурса и начнет скачивать образ. Скорость работы данного этапа зависит, в основном, от скорости вашего интернета - просто ожидайте, когда процесс получения образа завершиться.

---

:information_source: Если вас не устраивает скорость работы зеркал или испытываете трудности с получением образа, [обязательно ознакомьтесь с этим материалом](https://github.com/cbsd/mirrors).

---


![cbsd_local4.png](https://myb.convectix.com/img/cbsd_local4.png?raw=true)

Образ, на основе которого будет создана ваша ВМ будет сохранена и закеширована, поэтому повтороное использование данного профиля приведет к более быстрому созданию ВМ, поскольку скачивать образ уже не потребуется.
По окончанию процесса создании ВМ, диалог возвращает вас в привычный основной экран приложения.

![cbsd_local5.png](https://myb.convectix.com/img/cbsd_local5.png?raw=true)

Чтобы начать работу в ВМ, можете перейти к главе [Работа с ВМ](myb-qt-vm.md), либо прочесть о двух других методах работы с гипервизорами, если у вас есть в наличии выделенные сервера на базе [FreeBSD OS](https://www.freebsd.org/)/[Linux](https://kernel.org/) или используете дистрибутив [MyBee](https://myb.convectix.com).


---

**<<_**__[Назад: Режим работы N1: локальный CBSD интерпретатор](myb-qt-cbsd-local.md)__ $~~~$ | $~~~$ __[Дальше: Режим работы N3: через RestAPI](myb-qt-cbsd-api.md)__**_>>**

