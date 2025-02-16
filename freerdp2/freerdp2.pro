#
# Build FreeRDP2 static library
#
include(../common.pri)

TEMPLATE = aux
FREERDP_ARC = freerdp-2.11.7.tar.gz
FREERDP_DIR = $$section(FREERDP_ARC, '.', 0, 2)
FREERDP_URL = "https://pub.freerdp.com/releases/$${FREERDP_ARC}"
FREERDP_SRC = $${PWD}/$${FREERDP_ARC}

!exists($${FREERDP_SRC}) {
    FREERDP_CMD += ($$shell_quote($${CURL_COMMAND}) $${CURL_OPTIONS} -o \
        $$relative_path($${FREERDP_SRC}, $${OUT_PWD}) $${FREERDP_URL}) &&
}

!exists($${OUT_PWD}/$${FREERDP_DIR}/CMakeLists.txt) {
    FREERDP_CMD += ($$shell_quote($${TAR_COMMAND}) -xf $$relative_path($${FREERDP_SRC}, $${OUT_PWD})) &&
    FREERDP_CMD += ($$shell_quote($${PATCH_COMMAND}) -bp0 < $$shell_path($${PWD}/freerdp2.patch)) &&
}

# Unfortunately, FreeRDP2 for Windows does not support static build.
windows: BUILD_SHARED_LIBS = ON
else:    BUILD_SHARED_LIBS = OFF

FREERDP_CMD += ($${CMAKE_CONFIG_LIBS} -DBUILD_SHARED_LIBS=$${BUILD_SHARED_LIBS} \
    $$cat(BuildOptions.txt) -S $${FREERDP_DIR})
FREERDP_CMD += && $${CMAKE_BUILD_LIBS}

windows: FREERDP_LIB.target = ../lib/freerdp2.lib
else:    FREERDP_LIB.target = ../lib/libfreerdp2.$${QMAKE_EXTENSION_STATICLIB}
FREERDP_LIB.depends = $${PWD}/BuildOptions.txt
FREERDP_LIB.commands = $${FREERDP_CMD}
QMAKE_EXTRA_TARGETS += FREERDP_LIB
PRE_TARGETDEPS += $$FREERDP_LIB.target

DISTFILES += BuildOptions.txt
