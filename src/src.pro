#
# Build main executable
#
include(../common.pri)

TEMPLATE = app
macx: TARGET = $${APP_DISPLAYNAME}
else: TARGET = $${APP_NAME}
CONFIG -= qtquickcompiler
CONFIG += lrelease embed_translations
QT += qml quick quickcontrols2 network multimedia
qtHaveModule(core5compat): QT += core5compat

# The SshChannelExec class requires a private QIODevice header to call setReadChannelCount() for stdout/stderr
QT += core-private

android {
    lessThan(QT_MAJOR_VERSION,6) {
        QT += androidextras
        ANDROID_PACKAGE_SOURCE_DIR = $${PWD}/android-qt5
    } else {
        ANDROID_PACKAGE_SOURCE_DIR = $${PWD}/android-qt6
    }
    ANDROID_TARGET_SDK_VERSION = 34
    ANDROID_VERSION_NAME = $${APP_VERSION}
    ANDROID_VERSION_CODE = $$replace(ANDROID_VERSION_NAME, '\.', '')
    ANDROID_EXTRA_LIBS += $${OPENSSL_ROOT_DIR}/libssl_3.so $${OPENSSL_ROOT_DIR}/libcrypto_3.so
    INCLUDEPATH += \"$${OPENSSL_ROOT_DIR}/include\"
    LIBS += -L\"$${OPENSSL_ROOT_DIR}\" -lOpenSLES
    FREERDP_CHANNEL_LIBS = ../*.$${QMAKE_EXTENSION_STATICLIB}
} else {
    FREERDP_CHANNEL_LIBS = ../lib/freerdp2/*.$${QMAKE_EXTENSION_STATICLIB}
}

DEFINES += LIBSSH_STATIC SSH_NO_CPP_EXCEPTIONS

unix {
    INCLUDEPATH += \"$${OUT_PWD}/../include\" \"$${OUT_PWD}/../include/freerdp2\" \"$${OUT_PWD}/../include/winpr2\"
    LIBS += -L../lib -lssh -lvncclient
    LIBS += -lfreerdp-client2 $${FREERDP_CHANNEL_LIBS} -lfreerdp2 -lwinpr2
    LIBS += -lcrypto -lssl -lz -ldl -lutil
    #LIBS += -lcrypto -lssl -lcairo -ljpeg -lpng -llzo2 -lz
    #LIBS += $${QMAKE_LIBS_X11} -lxkbfile
}

SOURCES += \
    main.cpp \
    DesktopAction.cpp \
    DesktopClient.cpp \
    DesktopView.cpp \
    DirSpaceUsed.cpp \
    HttpRequest.cpp \
    KeyLoader.cpp \
    Parser.cpp \
    PtyIFace.cpp \
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
    SystemSignal.cpp \
    Terminal.cpp \
    TextRender.cpp \
    UrlModel.cpp \
    VncDesktopClient.cpp

HEADERS += \
    BaseThread.h \
    DesktopAction.h \
    DesktopClient.h \
    DesktopView.h \
    DirSpaceUsed.h \
    HttpRequest.h \
    KeyLoader.h \
    Parser.h \
    PtyIFace.h \
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
    SystemSignal.h \
    Terminal.h \
    TextRender.h \
    UrlModel.h \
    VncDesktopClient.h

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
