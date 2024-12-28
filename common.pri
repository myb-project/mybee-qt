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

QMAKE_CFLAGS += -fPIC
QMAKE_CFLAGS_RELEASE += \
    -fdata-sections -ffunction-sections \
    -Wno-pointer-sign \
    -Wno-deprecated-declarations \
    -Wno-tautological-constant-out-of-range-compare \
    -Wno-incompatible-pointer-types \
    -Wno-incompatible-pointer-types-discards-qualifiers

QMAKE_CXXFLAGS += -fPIC
QMAKE_CXXFLAGS_RELEASE += -fdata-sections -ffunction-sections

QMAKE_LFLAGS_APP += -Wl,--no-undefined
QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections

# Suppress the default RPATH
#QMAKE_LFLAGS_RPATH =
#QMAKE_LFLAGS_RPATHLINK =

win32: WHERE_COMMAND = where
else:  WHERE_COMMAND = which

COPY_COMMAND = $${QMAKE_COPY} -fpL

TAR_COMMAND = $$system($${WHERE_COMMAND} tar)
!isEmpty(TAR_COMMAND) {
    TAR_COMMAND += --strip-components=1 -xf
} else {
    error("The tar command was not found in the system path")
}

PATCH_COMMAND = $$system($${WHERE_COMMAND} patch)
!isEmpty(PATCH_COMMAND) {
    PATCH_COMMAND += -p0 <
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

CMAKE_CONFIG_LIBS = \
    $${CMAKE_COMMAND} -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SKIP_INSTALL_ALL_DEPENDENCY=ON \
    -DCMAKE_INSTALL_PREFIX=\"$${OUT_PWD}/..\" \
    -DCMAKE_C_COMPILER=\"$${QMAKE_CC}\" \
    -DCMAKE_C_FLAGS=\"$${QMAKE_CFLAGS}\" \
    -DCMAKE_C_FLAGS_RELEASE=\"$${QMAKE_CFLAGS_RELEASE}\" \
    $${OS_SPECIFIC_DEFINES}

CMAKE_BUILD_LIBS = \
    $${CMAKE_COMMAND} --build build && \
    $${CMAKE_COMMAND} --install build
