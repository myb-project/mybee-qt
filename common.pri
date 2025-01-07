1#
# The following APP_* variables can be freely modified to suit your taste before compiling the source code
#
APP_NAME        = mybee-qt
APP_DISPLAYNAME = MyBee-Qt
APP_VERSION     = $$cat(APP_VERSION)
APP_PUBLISHER   = "CBSD MyBee-Qt"
APP_HOMEURL     = "https://github.com/cbsd/cbsd"
APP_RELEASE     = "CBSD_MyBee-Qt_$${APP_VERSION}"

macx: APP_RELEASE = "$${APP_RELEASE}-universal"
else: APP_RELEASE = "$${APP_RELEASE}-x86_64"
APP_SOURCE_DIR  = $${PWD}/src
APP_STAGING_DIR = AppDir

DEFINES += APP_NAME='\\"$${APP_NAME}\\"'
DEFINES += APP_VERSION='\\"$${APP_VERSION}\\"'
DEFINES += APP_DISPLAYNAME='\\"$${APP_DISPLAYNAME}\\"'
DEFINES += APP_HOMEURL='\\"$${APP_HOMEURL}\\"'

CMAKE_BUILD_TYPE = Release

unix {
    QMAKE_CXXFLAGS += -fPIC
    QMAKE_CXXFLAGS_RELEASE += -fdata-sections -ffunction-sections

    QMAKE_CFLAGS += $${QMAKE_CXXFLAGS}
    QMAKE_CFLAGS_RELEASE += $${QMAKE_CXXFLAGS_RELEASE} \
        -Wno-pointer-sign \
        -Wno-deprecated-declarations \
        -Wno-tautological-constant-out-of-range-compare \
        -Wno-incompatible-pointer-types \
        -Wno-incompatible-pointer-types-discards-qualifiers

    QMAKE_LFLAGS_APP += -Wl,--no-undefined
    QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections

    # Suppress the default RPATH
    #QMAKE_LFLAGS_RPATH =
    #QMAKE_LFLAGS_RPATHLINK =
}

windows: WHERE_COMMAND = where
else:  WHERE_COMMAND = which

COPY_COMMAND = $${QMAKE_COPY}
!windows: COPY_COMMAND += -pL

TAR_COMMAND = $$system($${WHERE_COMMAND} tar)
!isEmpty(TAR_COMMAND) {
    TAR_COMMAND += --force-local -xf
} else {
    error("The tar command was not found in the system path")
}

PATCH_COMMAND = $$system($${WHERE_COMMAND} patch)
!isEmpty(PATCH_COMMAND) {
    PATCH_COMMAND += -bp0 <
} else {
    error("The patch command was not found in the system path")
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
    error("The cmake executable was not found in the system path")
}

android {
    QMAKE_CFLAGS += -D__USE_BSD
    QMAKE_CFLAGS_RELEASE += -Wno-unused-command-line-argument

    OPENSSL_ROOT_DIR = $$(OPENSSL_ROOT_DIR)
    isEmpty(OPENSSL_ROOT_DIR): OPENSSL_ROOT_DIR = $$(ANDROID_SDK_ROOT)/android_openssl/ssl_3/$${ANDROID_TARGET_ARCH}

    ANDROID_SDK_ROOT = $$(ANDROID_SDK_ROOT)
    isEmpty(ANDROID_SDK_ROOT): ANDROID_SDK_ROOT = $$(HOME)/Android/Sdk

    ANDROID_NDK_ROOT = $$(ANDROID_NDK_ROOT)
    isEmpty(ANDROID_NDK_ROOT): ANDROID_NDK_ROOT = $${ANDROID_SDK_ROOT}/ndk/25.1.8937393

    ANDROID_NDK_PLATFORM = $$(ANDROID_NDK_PLATFORM)
    isEmpty(ANDROID_NDK_PLATFORM): ANDROID_NDK_PLATFORM = 24 # getifaddrs() and IPv6 ifname

    OS_SPECIFIC_DEFINES = \
        -DCMAKE_FIND_ROOT_PATH=\"$${OPENSSL_ROOT_DIR}\" \
        -DOPENSSL_CRYPTO_LIBRARY=\"$${OPENSSL_ROOT_DIR}/libcrypto_3.so\" \
        -DANDROID_ABI=$${ANDROID_TARGET_ARCH} \
        -DANDROID_NATIVE_API_LEVEL=$${ANDROID_NDK_PLATFORM} \
        -DANDROID_NDK=\"$${ANDROID_NDK_ROOT}\" \
        -DCMAKE_TOOLCHAIN_FILE=\"$${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake\"
}

freebsd {
    OS_SPECIFIC_DEFINES = \
        -DCMAKE_MAKE_PROGRAM=/usr/bin/make
}

windows {
    VCVARSALL_BAT = $$(VCVARSALL_BAT)
    isEmpty(VCVARSALL_BAT): \
        VCVARSALL_BAT = $$shell_quote(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat) amd64

    CMAKE_CONFIG_LIBS = $${VCVARSALL_BAT} &&
    CMAKE_BUILD_LIBS = $${VCVARSALL_BAT} &&

    OPENSSL_ROOT_DIR = $$(OPENSSL_ROOT_DIR)
    isEmpty(OPENSSL_ROOT_DIR): \
        OPENSSL_ROOT_DIR = D:\OpenSSL-Win64

    OS_SPECIFIC_DEFINES = \
        -DOPENSSL_ROOT_DIR=\"$${OPENSSL_ROOT_DIR}\"
}

CMAKE_CONFIG_LIBS += \
    $$shell_quote($${CMAKE_COMMAND}) -B build \
    -DCMAKE_BUILD_TYPE=$${CMAKE_BUILD_TYPE} \
    -DCMAKE_SKIP_INSTALL_ALL_DEPENDENCY=ON \
    -DCMAKE_INSTALL_PREFIX=\"$${OUT_PWD}/..\" \
    -DCMAKE_C_COMPILER=\"$${QMAKE_CC}\" \
    -DCMAKE_C_FLAGS=\"$${QMAKE_CFLAGS}\" \
    -DCMAKE_C_FLAGS_RELEASE=\"$${QMAKE_CFLAGS_RELEASE}\" \
    $${OS_SPECIFIC_DEFINES}

CMAKE_BUILD_LIBS += \
    $$shell_quote($${CMAKE_COMMAND}) --build build --config $${CMAKE_BUILD_TYPE} && \
    $$shell_quote($${CMAKE_COMMAND}) --install build --config $${CMAKE_BUILD_TYPE}

