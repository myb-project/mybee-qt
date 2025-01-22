1#
# The following APP_* variables can be freely modified to suit your taste before compiling the source code
#
APP_NAME        = mybee-qt
APP_DISPLAYNAME = MyBee-Qt
APP_VERSION     = $$cat(APP_VERSION)
APP_PUBLISHER   = "CBSD MyBee-Qt"
APP_HOMEURL     = "https://github.com/cbsd/cbsd"
APP_RELEASE     = "$${APP_DISPLAYNAME}_$${APP_VERSION}"

macx: APP_RELEASE = "$${APP_RELEASE}-universal"
else: APP_RELEASE = "$${APP_RELEASE}-x86_64"
APP_STAGING_DIR = AppDir

DEFINES += APP_NAME='\\"$${APP_NAME}\\"'
DEFINES += APP_VERSION='\\"$${APP_VERSION}\\"'
DEFINES += APP_DISPLAYNAME='\\"$${APP_DISPLAYNAME}\\"'
DEFINES += APP_HOMEURL='\\"$${APP_HOMEURL}\\"'

CMAKE_BUILD_TYPE = Release

windows: WHERE_COMMAND = where
else:    WHERE_COMMAND = which

CMAKE_COMMAND = $$system($${WHERE_COMMAND} cmake)
isEmpty(CMAKE_COMMAND): error("The cmake executable was not found in the system path")

CURL_COMMAND = $$system($${WHERE_COMMAND} curl)
isEmpty(CURL_COMMAND): error("The curl executable was not found in the system path")

TAR_COMMAND = $$system($${WHERE_COMMAND} tar)
isEmpty(TAR_COMMAND): error("The tar executable was not found in the system path")

#PATCH_COMMAND = $$system($${WHERE_COMMAND} patch)
#isEmpty(PATCH_COMMAND): error("The patch executable was not found in the system path")

windows {
    VCVARSALL_BAT = $$(VCVARSALL_BAT)
    isEmpty(VCVARSALL_BAT): VCVARSALL_BAT = \
        $$shell_quote(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat) amd64

    CMAKE_PREFIX_PATH = "$${PWD}/win64/libjpeg;$${PWD}/win64/zlib"

    OPENSSL_ROOT_DIR = $$(OPENSSL_ROOT_DIR)
    isEmpty(OPENSSL_ROOT_DIR): OPENSSL_ROOT_DIR = "D:\OpenSSL-Win64"

    CMAKE_CONFIG_LIBS = $${VCVARSALL_BAT} && $$shell_quote($${CMAKE_COMMAND}) \
        -DCMAKE_PREFIX_PATH=$$shell_quote($${CMAKE_PREFIX_PATH}) \
        -DOPENSSL_ROOT_DIR=$$shell_quote($${OPENSSL_ROOT_DIR})

    QMAKE_CXXFLAGS += /MP
    QMAKE_CXXFLAGS_WARN_ON += /WX /W3
    QMAKE_CXXFLAGS_RELEASE += /wd4267

    QMAKE_CFLAGS_RELEASE = /O2 /wd4267

} else { # linux & unix

    CMAKE_CONFIG_LIBS = $$shell_quote($${CMAKE_COMMAND}) \
        -DCMAKE_C_COMPILER=$$shell_quote($${QMAKE_CC})

    QMAKE_CXXFLAGS += -fPIC
    QMAKE_CXXFLAGS_RELEASE += -fdata-sections -ffunction-sections

    QMAKE_CFLAGS += -fPIC
    QMAKE_CFLAGS_RELEASE += -fdata-sections -ffunction-sections \
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

    android {
        QMAKE_CFLAGS += -D__USE_BSD
        QMAKE_CFLAGS_RELEASE += -Wno-unused-command-line-argument

        OPENSSL_ROOT_DIR = $$(OPENSSL_ROOT_DIR)
        isEmpty(OPENSSL_ROOT_DIR): OPENSSL_ROOT_DIR = \
            $$(ANDROID_SDK_ROOT)/android_openssl/ssl_3/$${ANDROID_TARGET_ARCH}

        ANDROID_SDK_ROOT = $$(ANDROID_SDK_ROOT)
        isEmpty(ANDROID_SDK_ROOT): ANDROID_SDK_ROOT = $$(HOME)/Android/Sdk

        ANDROID_NDK_ROOT = $$(ANDROID_NDK_ROOT)
        isEmpty(ANDROID_NDK_ROOT): ANDROID_NDK_ROOT = $${ANDROID_SDK_ROOT}/ndk/25.1.8937393

        ANDROID_NDK_PLATFORM = $$(ANDROID_NDK_PLATFORM)
        isEmpty(ANDROID_NDK_PLATFORM): ANDROID_NDK_PLATFORM = 24 # getifaddrs() and IPv6 ifname

        CMAKE_CONFIG_LIBS += \
            -DCMAKE_FIND_ROOT_PATH=$${OPENSSL_ROOT_DIR} \
            -DOPENSSL_CRYPTO_LIBRARY=$${OPENSSL_ROOT_DIR}/libcrypto_3.so \
            -DANDROID_ABI=$${ANDROID_TARGET_ARCH} \
            -DANDROID_NATIVE_API_LEVEL=$${ANDROID_NDK_PLATFORM} \
            -DANDROID_NDK=$${ANDROID_NDK_ROOT} \
            -DCMAKE_TOOLCHAIN_FILE=$${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake
    } else {
        CMAKE_CONFIG_LIBS += \
            -DCMAKE_MAKE_PROGRAM=/usr/bin/make
    }

    CMAKE_CONFIG_LIBS += \
        -DCMAKE_C_FLAGS=$$shell_quote($${QMAKE_CFLAGS})
}

CMAKE_CONFIG_LIBS += -B build \
    -DCMAKE_C_FLAGS_RELEASE=$$shell_quote($${QMAKE_CFLAGS_RELEASE}) \
    -DCMAKE_INSTALL_PREFIX=$$shell_quote($${OUT_PWD}/..) \
    -DCMAKE_BUILD_TYPE=$${CMAKE_BUILD_TYPE} \
    -DCMAKE_SKIP_INSTALL_ALL_DEPENDENCY=ON

CMAKE_BUILD_LIBS = \
    $$shell_quote($${CMAKE_COMMAND}) --build build --config $${CMAKE_BUILD_TYPE} && \
    $$shell_quote($${CMAKE_COMMAND}) --install build --config $${CMAKE_BUILD_TYPE}

#message($${CMAKE_CONFIG_LIBS})
