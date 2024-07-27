#
# Build VNCClient static library
#
include(../common.pri)

TEMPLATE = aux
VNCCLIENT_ARC = LibVNCServer-0.9.14.tar.gz
VNCCLIENT_URL = "https://github.com/LibVNC/libvncserver/archive/refs/tags/$${VNCCLIENT_ARC}"
VNCCLIENT_SRC = $${PWD}/$${VNCCLIENT_ARC}

!exists($${VNCCLIENT_SRC}): \
    VNCCLIENT_CMD += ($${WGET_COMMAND} \"$${VNCCLIENT_SRC}\" $${VNCCLIENT_URL}) &&
!exists($${OUT_PWD}/CMakeLists.txt): \
    VNCCLIENT_CMD += ($${TAR_COMMAND} \"$${VNCCLIENT_SRC}\") &&

VNCCLIENT_CMD += ($${CMAKE_CONFIG_LIBS} $$cat(BuildOptions.txt)) &&
VNCCLIENT_CMD += $${CMAKE_BUILD_LIBS}

VNCCLIENT_LIB.target = ../lib/libvncclient.$${QMAKE_EXTENSION_STATICLIB}
VNCCLIENT_LIB.depends = $${PWD}/BuildOptions.txt
VNCCLIENT_LIB.commands = $${VNCCLIENT_CMD}
QMAKE_EXTRA_TARGETS += VNCCLIENT_LIB
PRE_TARGETDEPS += $$VNCCLIENT_LIB.target

#message($${VNCCLIENT_CMD})

DISTFILES += BuildOptions.txt
