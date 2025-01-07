#
# Build libssh static library
#
include(../common.pri)

TEMPLATE = aux
LIBSSH_ARC = libssh-0.11.1.tar.xz
LIBSSH_DIR = $$section(LIBSSH_ARC, ., 0, 2)
LIBSSH_URL = "https://www.libssh.org/files/0.11/$${LIBSSH_ARC}"
LIBSSH_SRC = $${PWD}/$${LIBSSH_ARC}

!exists($${LIBSSH_SRC}): \
    LIBSSH_CMD += ($${WGET_COMMAND} \"$${LIBSSH_SRC}\" $${LIBSSH_URL}) &&
!exists($${OUT_PWD}/$${LIBSSH_DIR}/CMakeLists.txt): \
    LIBSSH_CMD += ($${TAR_COMMAND} \"$${LIBSSH_SRC}\") &&

LIBSSH_CMD += ($${CMAKE_CONFIG_LIBS} $$cat(BuildOptions.txt) -S $${LIBSSH_DIR})
LIBSSH_CMD += && $${CMAKE_BUILD_LIBS}

windows: LIBSSH_LIB.target = ../lib/ssh.lib
else:    LIBSSH_LIB.target = ../lib/libssh.$${QMAKE_EXTENSION_STATICLIB}
LIBSSH_LIB.depends = $${PWD}/BuildOptions.txt
LIBSSH_LIB.commands = $${LIBSSH_CMD}
QMAKE_EXTRA_TARGETS += LIBSSH_LIB
PRE_TARGETDEPS += $$LIBSSH_LIB.target

DISTFILES += BuildOptions.txt
