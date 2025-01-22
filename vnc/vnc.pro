#
# Build VNCClient static library
#
include(../common.pri)

TEMPLATE = aux
VNCCLIENT_ARC = LibVNCServer-0.9.15.tar.gz
VNCCLIENT_DIR = $$section(VNCCLIENT_ARC, '.', 0, 2)
VNCCLIENT_URL = "https://codeload.github.com/LibVNC/libvncserver/tar.gz/refs/tags/$${VNCCLIENT_DIR}"
VNCCLIENT_SRC = $${PWD}/$${VNCCLIENT_ARC}

!exists($${VNCCLIENT_SRC}) {
    VNCCLIENT_CMD += ($$shell_quote($${CURL_COMMAND}) -o \
        $$relative_path($${VNCCLIENT_SRC}, $${OUT_PWD}) $${VNCCLIENT_URL}) &&
}

!exists($${OUT_PWD}/libvncserver-$${VNCCLIENT_DIR}/CMakeLists.txt) {
    VNCCLIENT_CMD += ($$shell_quote($${TAR_COMMAND}) -xf \
        $$relative_path($${VNCCLIENT_SRC}, $${OUT_PWD})) &&
}

VNCCLIENT_CMD += ($${CMAKE_CONFIG_LIBS} $$cat(BuildOptions.txt) -S libvncserver-$${VNCCLIENT_DIR})
VNCCLIENT_CMD += && $${CMAKE_BUILD_LIBS}

windows: VNCCLIENT_LIB.target = ../lib/vncclient.lib
else:    VNCCLIENT_LIB.target = ../lib/libvncclient.$${QMAKE_EXTENSION_STATICLIB}
VNCCLIENT_LIB.depends = $${PWD}/BuildOptions.txt
VNCCLIENT_LIB.commands = $${VNCCLIENT_CMD}
QMAKE_EXTRA_TARGETS += VNCCLIENT_LIB
PRE_TARGETDEPS += $$VNCCLIENT_LIB.target

DISTFILES += BuildOptions.txt
