TEMPLATE = subdirs

freebsd {
    SUBDIRS = src
} else {
    SUBDIRS = zlib-ng libjpeg-turbo libssh freerdp2 vnc src
    libssh.depends = zlib-ng
    freerdp2.depends = zlib-ng libjpeg-turbo
    vnc.depends = zlib-ng libjpeg-turbo
    src.depends = libssh freerdp2 vnc
}

#CONFIG += ordered

DISTFILES += \
    APP_VERSION \
    common.pri \
    LICENSE \
    lupdate-ru_RU.sh \
    mybee-qt.pro \
    README.md \
    README.ru.md \
    $$files(deploy/*, true) \
    $$files(docs/*, true)
