#
# Build zlib-ng static library
#
include(../common.pri)

TEMPLATE = aux
ZLIBNG_VER = 2.2.3
ZLIBNG_ARC = zlib-ng-$${ZLIBNG_VER}.tar.gz
ZLIBNG_DIR = $$section(ZLIBNG_ARC, '.', 0, 2)
ZLIBNG_URL = "https://github.com/zlib-ng/zlib-ng/archive/refs/tags/$${ZLIBNG_VER}.tar.gz"
ZLIBNG_SRC = $${PWD}/$${ZLIBNG_ARC}

!exists($${ZLIBNG_SRC}) {
    ZLIBNG_CMD += ($$shell_quote($${CURL_COMMAND}) $${CURL_OPTIONS} -o \
        $$relative_path($${ZLIBNG_SRC}, $${OUT_PWD}) $${ZLIBNG_URL}) &&
}

!exists($${OUT_PWD}/$${ZLIBNG_DIR}/CMakeLists.txt) {
    ZLIBNG_CMD += ($$shell_quote($${TAR_COMMAND}) -xf \
        $$relative_path($${ZLIBNG_SRC}, $${OUT_PWD})) &&
}

ZLIBNG_CMD += ($${CMAKE_CONFIG_LIBS} $$cat(BuildOptions.txt) -S $${ZLIBNG_DIR})
ZLIBNG_CMD += && $${CMAKE_BUILD_LIBS}

windows: ZLIBNG_LIB.target = ../lib/z.lib
else:    ZLIBNG_LIB.target = ../lib/libz.$${QMAKE_EXTENSION_STATICLIB}
ZLIBNG_LIB.depends = $${PWD}/BuildOptions.txt
ZLIBNG_LIB.commands = $${ZLIBNG_CMD}
QMAKE_EXTRA_TARGETS += ZLIBNG_LIB
PRE_TARGETDEPS += $$ZLIBNG_LIB.target

DISTFILES += BuildOptions.txt
