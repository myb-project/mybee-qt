#
# Build libjpeg-turbo static library
#
include(../common.pri)

TEMPLATE = aux
LIBJPEG_VER = 3.1.0
LIBJPEG_ARC = libjpeg-turbo-$${LIBJPEG_VER}.tar.gz
LIBJPEG_DIR = $$section(LIBJPEG_ARC, '.', 0, 2)
LIBJPEG_URL = "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/$${LIBJPEG_VER}.tar.gz"
LIBJPEG_SRC = $${PWD}/$${LIBJPEG_ARC}

!exists($${LIBJPEG_SRC}) {
    LIBJPEG_CMD += ($$shell_quote($${CURL_COMMAND}) $${CURL_OPTIONS} -o \
        $$relative_path($${LIBJPEG_SRC}, $${OUT_PWD}) $${LIBJPEG_URL}) &&
}

!exists($${OUT_PWD}/$${LIBJPEG_DIR}/CMakeLists.txt) {
    LIBJPEG_CMD += ($$shell_quote($${TAR_COMMAND}) -xf \
        $$relative_path($${LIBJPEG_SRC}, $${OUT_PWD})) &&
}

LIBJPEG_CMD += ($${CMAKE_CONFIG_LIBS} $$cat(BuildOptions.txt) -S $${LIBJPEG_DIR})
LIBJPEG_CMD += && $${CMAKE_BUILD_LIBS}

windows: LIBJPEG_LIB.target = ../lib/turbojpeg.lib
else:    LIBJPEG_LIB.target = ../lib/libturbojpeg.$${QMAKE_EXTENSION_STATICLIB}
LIBJPEG_LIB.depends = $${PWD}/BuildOptions.txt
LIBJPEG_LIB.commands = $${LIBJPEG_CMD}
QMAKE_EXTRA_TARGETS += LIBJPEG_LIB
PRE_TARGETDEPS += $$LIBJPEG_LIB.target

DISTFILES += BuildOptions.txt
