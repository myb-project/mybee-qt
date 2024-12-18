TEMPLATE = subdirs
SUBDIRS = libssh freerdp2 vnc src
CONFIG += c++17 release

#CONFIG += ordered
src.depends = libssh freerdp2 vnc

DISTFILES += \
    APP_VERSION \
    common.pri \
    LICENSE \
    lupdate-ru_RU.sh \
    mybee-qt.pro \
    README.md
