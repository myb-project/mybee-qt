#
# Build VNCClient static library
#
include(../common.pri)

TEMPLATE = aux
VNCCLIENT_ARC = LibVNCServer-0.9.15.tar.gz
VNCCLIENT_DIR = libvncserver-$$section(VNCCLIENT_ARC, ., 0, 2)
VNCCLIENT_URL = "https://github.com/LibVNC/libvncserver/archive/refs/tags/$${VNCCLIENT_ARC}"
VNCCLIENT_SRC = $${PWD}/$${VNCCLIENT_ARC}

!exists($${VNCCLIENT_SRC}): \
    VNCCLIENT_CMD += ($${WGET_COMMAND} \"$${VNCCLIENT_SRC}\" $${VNCCLIENT_URL}) &&
!exists($${OUT_PWD}/$${VNCCLIENT_DIR}/CMakeLists.txt) {
    VNCCLIENT_CMD += ($${TAR_COMMAND} \"$${VNCCLIENT_SRC}\") &&
    windows: VNCCLIENT_CMD += (cd $${VNCCLIENT_DIR} && $${PATCH_COMMAND} \"$${PWD}/timeval.patch\" && cd ..) &&
}

VNCCLIENT_CMD += ($${CMAKE_CONFIG_LIBS} $$cat(BuildOptions.txt) -S $${VNCCLIENT_DIR})
VNCCLIENT_CMD += && $${CMAKE_BUILD_LIBS}

windows: VNCCLIENT_LIB.target = ../lib/vncclient.lib
else:    VNCCLIENT_LIB.target = ../lib/libvncclient.$${QMAKE_EXTENSION_STATICLIB}
VNCCLIENT_LIB.depends = $${PWD}/BuildOptions.txt
VNCCLIENT_LIB.commands = $${VNCCLIENT_CMD}
QMAKE_EXTRA_TARGETS += VNCCLIENT_LIB
PRE_TARGETDEPS += $$VNCCLIENT_LIB.target

DISTFILES += \
    BuildOptions.txt \
    $$files(*.patch)
