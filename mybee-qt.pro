QT += network qml quick quickcontrols2 multimedia
greaterThan(QT_MAJOR_VERSION,5): QT += core5compat

# class QIODevice private header required
QT += core-private

CONFIG -= qtquickcompiler
CONFIG += lrelease embed_translations

APP_NAME = mybee-qt
APP_VERSION = $$cat(VERSION)
DEFINES += APP_NAME='\\"$${APP_NAME}\\"'
DEFINES += APP_VERSION='\\"$${APP_VERSION}\\"'

QLIBSSH2_SRC = qlibssh2/src
INCLUDEPATH += $${QLIBSSH2_SRC}

android {
    QT += androidextras
    ANDROID_EXTRA_LIBS += \
        $${PWD}/android/openssl/lib/libcrypto_1_1.so \
        $${PWD}/android/openssl/lib/libssl_1_1.so
    INCLUDEPATH += $${PWD}/android/libssh2/include
    LIBS += -L$${PWD}/android/openssl/lib -lcrypto_1_1 -lssl_1_1 $${PWD}/android/libssh2/lib/libssh2.a
} else {
    LIBS += -lcrypto -lssh2 -lvncclient -lfreerdp2 -lfreerdp-client2 -lwinpr2
    *bsd {
        LIBS += -lutil
        INCLUDEPATH += /usr/local/include/freerdp2 /usr/local/include/winpr2
    } else {
        INCLUDEPATH += /usr/include/freerdp2 /usr/include/winpr2
    }
}

SOURCES += \
    $${QLIBSSH2_SRC}/Ssh2Channel.cpp \
    $${QLIBSSH2_SRC}/Ssh2Client.cpp \
    $${QLIBSSH2_SRC}/Ssh2LocalPortForwarding.cpp \
    $${QLIBSSH2_SRC}/Ssh2Process.cpp \
    $${QLIBSSH2_SRC}/Ssh2Scp.cpp \
    $${QLIBSSH2_SRC}/Ssh2Settings.cpp \
    $${QLIBSSH2_SRC}/Ssh2Types.cpp \
    src/main.cpp \
    src/DesktopAction.cpp \
    src/DesktopClient.cpp \
    src/DesktopView.cpp \
    src/HttpRequest.cpp \
    src/KeyLoader.cpp \
    src/Parser.cpp \
    src/PtyIFace.cpp \
    src/RdpAudioPlugin.cpp \
    src/RdpDesktopClient.cpp \
    src/SshConnection.cpp \
    src/SshKeygenEd25519.cpp \
    src/SystemHelper.cpp \
    src/SystemProcess.cpp \
    src/SystemSignal.cpp \
    src/Terminal.cpp \
    src/TextRender.cpp \
    src/UrlModel.cpp \
    src/VncDesktopClient.cpp

HEADERS += \
    $${QLIBSSH2_SRC}/Ssh2Channel.h \
    $${QLIBSSH2_SRC}/Ssh2Client.h \
    $${QLIBSSH2_SRC}/Ssh2LocalPortForwarding.h \
    $${QLIBSSH2_SRC}/Ssh2Process.h \
    $${QLIBSSH2_SRC}/Ssh2Scp.h \
    $${QLIBSSH2_SRC}/Ssh2Settings.h \
    $${QLIBSSH2_SRC}/Ssh2Types.h \
    src/BaseThread.h \
    src/DesktopAction.h \
    src/DesktopClient.h \
    src/DesktopView.h \
    src/HttpRequest.h \
    src/KeyLoader.h \
    src/Parser.h \
    src/PtyIFace.h \
    src/RdpAudioPlugin.h \
    src/RdpDesktopClient.h \
    src/SshConnection.h \
    src/SystemHelper.h \
    src/SystemProcess.h \
    src/SystemSignal.h \
    src/Terminal.h \
    src/TextRender.h \
    src/UrlModel.h \
    src/VncDesktopClient.h

RESOURCES += \
    icons.qrc \
    images.qrc \
    qml.qrc \
    resource.qrc

TRANSLATIONS += $$files(translations/$${APP_NAME}-*.ts)

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
