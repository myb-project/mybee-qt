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
pkg install -y cbsd
```

2) Ининциализация CBSD:
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

3) Создать пользовалеля, от которого будем работать. Не забудем включить его в группу `cbsd` на вопросе (Invite into other groups):
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
mybee      ALL=(ALL) NOPASSWD:SETENV: MYB_CBSD_CMD
```
, где `mybee` - login вашего пользователя.

:bangbang: | :Внимание: Данная настройка дает пользователю 'sshuser' полномочия 'root' на хостовой системе через /usr/local/bin интерпретатор!
:---: | :---

установите корректные права доступа на файл:

```
chmod 0400 /usr/local/etc/sudoers.d/20_mybee
```

5) Сгенерируйте приватный и публичный SSH ключ для пользователя `mybee`:
```
su - mybee -c "ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519 -N ''"
```

и скопируйте .pub ключ в authorized_keys:
```
cp -a ~mybee/.ssh/id_ed25519.pub ~mybee/.ssh/authorized_keys
```

6) Забираем приватный ключ `~mybee/.ssh/id_ed25519` на клиентский хост, на котором будем использовать MyBee-QT, положив его, например, в каталог ~/private под именем f14-mybee.key, не забыв установить безопасные права доступа:
```
mkdir ~/private
chmod 0700 ~/private
chmod 0600 ~/private/f14-mybee.key
```

7) Убедитесь, что вы соединяетесь по SSH от пользователя `mybee` с использованием приватного ключа и интерпретатор /usr/local/bin/cbsd доступен без ввода пароля:
```
ssh -i ~/private/f14-mybee.key mybee@<host> cbsd version
ssh -i ~/private/f14-mybee.key mybee@<host> sudo cbsd version
```

Если все в порядке, приложение готово к использованию.

8) В MyBee-QT нажмите кнопку [+] Create и в диалоговом окне используйте следующие элементы для конфигурирования подключения:

  - 1) добавьте и импортируйте приватный ключ удаленного хоста;
  - 2) ключ должен появиться в качестве доступных для выбора;
  - 3) укажите параметры подключения к удаленному хосту (в нашем примере это 172.16.0.91);
  - 4) метод работы с гипервизором - через SSH;

![mybqt-ssh5.png](https://myb.convectix.com/img/mybqt-ssh5.png?raw=true)

диалог выбора приватного ключа с файловой системы:

![mybqt-ssh4.png](https://myb.convectix.com/img/mybqt-ssh4.png?raw=true)

при нажатии кнопки 'Profile' вы переходите к выбору профилей, доступных на удаленном сервере. Если вы подключаетесь первый раз, вы увидете окно информационного характера с хешем ключа:

![mybqt-ssh6.png](https://myb.convectix.com/img/mybqt-ssh4.png?raw=true)

Если все в порядке, то на следующем экране вы увидите выбор доступных движков на удаленной системе, которые вы можете использовать. jail и bhyve входят в базовую ОС FreeBSD, поэтому вы увидите их сразу:

![mybqt-ssh7.png](https://myb.convectix.com/img/mybqt-ssh7.png?raw=true)


</details>

<details>
  <summary>Настройка Linux: AltLinux</summary>


</details>

<details>
  <summary>Настройка Linux: Debian/Ubuntu</summary>
</details>

<details>
  <summary>Настройка Linux: Manjaro</summary>
</details>



---

**<< Назад**: [Режим работы N1: локальный CBSD интерпретатор](myb-qt-cbsd-local.md)     |     **>> Дальше**: [Режим работы N3: через RestAPI](myb-qt-cbsd-api.md)
