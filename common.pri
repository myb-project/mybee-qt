APP_NAME        = mybee-qt
APP_DISPLAYNAME = MyBee-Qt
APP_VERSION     = $$cat(APP_VERSION)
APP_PUBLISHER   = "CBSD Store"
APP_HOMEURL     = "https://www.bsdstore.ru/"
APP_RELEASE     = "CBSD_MyBee-Qt_$${APP_VERSION}"
macx: APP_RELEASE = "$${APP_RELEASE}-universal"
else: APP_RELEASE = "$${APP_RELEASE}-x86_64"

DEFINES += APP_NAME='\\"$${APP_NAME}\\"'
DEFINES += APP_VERSION='\\"$${APP_VERSION}\\"'
CONFIG += c++17 release

QMAKE_CFLAGS += -fPIC
QMAKE_CFLAGS_RELEASE += \
    -fdata-sections -ffunction-sections \
    -Wno-pointer-sign \
    -Wno-deprecated-declarations \
    -Wno-tautological-constant-out-of-range-compare \
    -Wno-incompatible-pointer-types \
    -Wno-incompatible-pointer-types-discards-qualifiers \
    -Wno-implicit-const-int-float-conversion

QMAKE_CXXFLAGS += -fPIC
QMAKE_CXXFLAGS_RELEASE += -fdata-sections -ffunction-sections

QMAKE_LFLAGS_APP += -Wl,--no-undefined
QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections

# Suppress the default RPATH
#QMAKE_LFLAGS_RPATH =
#QMAKE_LFLAGS_RPATHLINK =

win32: WHERE_COMMAND = where
else:  WHERE_COMMAND = which

COPY_COMMAND = $${QMAKE_COPY} -upL

TAR_COMMAND = $$system($${WHERE_COMMAND} tar)
!isEmpty(TAR_COMMAND) {
    TAR_COMMAND += --strip-components=1 -xf
} else {
    error("The tar command was not found in the system path")
}

WGET_COMMAND = $$system($${WHERE_COMMAND} wget)
!isEmpty(WGET_COMMAND) {
    WGET_COMMAND += -O
} else {
    WGET_COMMAND = $$system($${WHERE_COMMAND} curl)
    !isEmpty(WGET_COMMAND) {
        WGET_COMMAND += -o
    } else {
        error("Neither wget nor curl commands were found in the system path")
    }
}

CMAKE_COMMAND = $$system($${WHERE_COMMAND} cmake)
isEmpty(CMAKE_COMMAND) {
    error("The cmake command was not found in the system path")
}

CMAKE_CONFIG_LIBS = \
    $${CMAKE_COMMAND} -B build \
#   -GNinja -DCMAKE_TOOLCHAIN_FILE=<full path to toolchain.cmake> -DANDROID_NDK=<path> -DANDROID_NATIVE_API_LEVEL=<API level you want> \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SKIP_INSTALL_ALL_DEPENDENCY=ON \
    -DCMAKE_INSTALL_PREFIX=\"$${OUT_PWD}/..\" \
    -DCMAKE_C_COMPILER=\"$${QMAKE_CC}\" \
    -DCMAKE_C_FLAGS=\"$${QMAKE_CFLAGS}\" \
    -DCMAKE_C_FLAGS_RELEASE=\"$${QMAKE_CFLAGS_RELEASE}\" \
#    -DCMAKE_CXX_COMPILER=\"$${QMAKE_CXX}\" \
#    -DCMAKE_CXX_FLAGS=\"$${QMAKE_CXXFLAGS}\" \
#    -DCMAKE_CXX_FLAGS_RELEASE=\"$${QMAKE_CXXFLAGS_RELEASE}\"

CMAKE_BUILD_LIBS = \
    $${CMAKE_COMMAND} --build build && \
    $${CMAKE_COMMAND} --install build
