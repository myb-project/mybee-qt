#
# Build main executable
#
include(../common.pri)

TEMPLATE = app
macx: TARGET = $${APP_DISPLAYNAME}
else: TARGET = $${APP_NAME}
# The SshChannelExec class requires a private QIODevice header to call setReadChannelCount() for stdout/stderr
QT += core-private
QT += qml quick quickcontrols2 network multimedia
qtHaveModule(core5compat): QT += core5compat

CONFIG += c++17 release
CONFIG += file_copies lrelease embed_translations

DEFINES += LIBSSH_STATIC SSH_NO_CPP_EXCEPTIONS

freebsd {
    INCLUDEPATH += /usr/local/include/freerdp2 /usr/local/include/freerdp2/freerdp/client  /usr/local/include/winpr2 ../include ../include/freerdp2 ../include/winpr2
} else {
    INCLUDEPATH += ../include ../include/freerdp2 ../include/winpr2
}

!isEmpty(OPENSSL_ROOT_DIR) {
    INCLUDEPATH += $$clean_path($${OPENSSL_ROOT_DIR}/include)
    QMAKE_LIBDIR += $$clean_path($${OPENSSL_ROOT_DIR}/lib)
}

windows {
    CONFIG += deploy
    RC_ICONS = $$shell_quote($${PWD}/../deploy/windows/logo.ico)

    QMAKE_LIBDIR += ../lib
    LIBS += ssh.lib
    LIBS += vncclient.lib
    LIBS += freerdp-client2.lib freerdp2.lib winpr2.lib
    LIBS += turbojpeg-static.lib zlibstatic.lib
    LIBS += libssl.lib libcrypto.lib
    LIBS += Advapi32.lib User32.lib Ws2_32.lib Iphlpapi.lib Ntdsapi.lib Rpcrt4.lib Dbghelp.lib

} else: macx { # XXX Not implemented yet!

    QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
    #QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.0
    QMAKE_TARGET_BUNDLE_PREFIX += cbsd.mybeeqt
    ICON = "../deploy/macos/logo.icns"
    DEFINES += USE_APPKIT USE_APPLICATION_SERVICES USE_IOKIT USE_OBJC
    LIBS += -framework Carbon -framework ApplicationServices -framework IOKit -framework AppKit -lobjc

} else: android {

    CONFIG -= qtquickcompiler
    lessThan(QT_MAJOR_VERSION,6) {
        QT += androidextras
        ANDROID_PACKAGE_SOURCE_DIR = $${PWD}/android-qt5
    } else: ANDROID_PACKAGE_SOURCE_DIR = $${PWD}/android-qt6

    ANDROID_TARGET_SDK_VERSION = 35
    ANDROID_VERSION_NAME = $${APP_VERSION}
    ANDROID_VERSION_CODE = $$replace(ANDROID_VERSION_NAME, '\.', '')
    ANDROID_EXTRA_LIBS += $${OPENSSL_ROOT_DIR}/libssl_3.so $${OPENSSL_ROOT_DIR}/libcrypto_3.so

    LIBS += ../lib/libssh.a ../lib/libvncclient.a
    LIBS += ../lib/libfreerdp-client2.a ../lib*.a ../lib/libfreerdp2.a ../lib/libwinpr2.a
    LIBS += ../lib/libturbojpeg.a ../lib/libz.a
    LIBS += -lssl -lcrypto -ldl -lOpenSLES

} else { # any other unix & linux

    freebsd {
        LIBS += -lssl -lcrypto -ldl -lutil -lturbojpeg -lz-ng -lssh -lfreerdp-client2 -lfreerdp2 -lwinpr2 -lvncclient -lz
    } else {
        LIBS += ../lib/libssh.a ../lib/libvncclient.a
        LIBS += ../lib/libfreerdp-client2.a ../lib/freerdp2/lib*.a ../lib/libfreerdp2.a ../lib/libwinpr2.a
        LIBS += ../lib/libturbojpeg.a ../lib/libz.a
        LIBS += -lssl -lcrypto -ldl -lutil
    }
}

SOURCES += \
    main.cpp \
    DesktopAction.cpp \
    DesktopClient.cpp \
    DesktopView.cpp \
    DirSpaceUsed.cpp \
    HttpRequest.cpp \
    Parser.cpp \
    RdpAudioPlugin.cpp \
    RdpDesktopClient.cpp \
    SshChannel.cpp \
    SshChannelExec.cpp \
    SshChannelFtp.cpp \
    SshChannelPort.cpp \
    SshChannelShell.cpp \
    SshKeygenEd25519.cpp \
    SshProcess.cpp \
    SshSession.cpp \
    SshSettings.cpp \
    SystemHelper.cpp \
    SystemProcess.cpp \
    Terminal.cpp \
    TextRender.cpp \
    UrlModel.cpp \
    VncDesktopClient.cpp \
    WindowState.cpp

HEADERS += \
    BaseThread.h \
    DesktopAction.h \
    DesktopClient.h \
    DesktopView.h \
    DirSpaceUsed.h \
    HttpRequest.h \
    Parser.h \
    RdpAudioPlugin.h \
    RdpDesktopClient.h \
    SshChannel.h \
    SshChannelExec.h \
    SshChannelFtp.h \
    SshChannelPort.h \
    SshChannelShell.h \
    SshProcess.h \
    SshSession.h \
    SshSettings.h \
    SystemHelper.h \
    SystemProcess.h \
    Terminal.h \
    TextRender.h \
    UrlModel.h \
    VncDesktopClient.h \
    WindowState.h

unix {
    SOURCES += \
        PtyIFace.cpp \
        SystemSignal.cpp

    HEADERS += \
        PtyIFace.h \
        SystemSignal.h
}

RESOURCES += \
    fonts.qrc \
    icons.qrc \
    images.qrc \
    qml.qrc \
    resource.qrc

OTHER_FILES += \
    qml/*.qml \

TRANSLATIONS += $$files(translations/$${APP_NAME}-*.ts)

DISTFILES += $$files(android-qt*, true) \
    android-qt6/AndroidManifest.xml \
    android-qt6/build.gradle \
    android-qt6/gradle.properties \
    android-qt6/gradle/wrapper/gradle-wrapper.jar \
    android-qt6/gradle/wrapper/gradle-wrapper.properties \
    android-qt6/gradlew \
    android-qt6/gradlew.bat \
    android-qt6/res/values/libs.xml \
    android-qt6/res/xml/qtprovider_paths.xml

CONFIG(deploy) {
    DEPLOY_PRI = ../deploy/deploy.pri
    exists("$${DEPLOY_PRI}"): include("$${DEPLOY_PRI}")
    else: error("Can't find $${DEPLOY_PRI}")
}
