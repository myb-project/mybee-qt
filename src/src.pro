#
# Build main executable
#
include(../common.pri)

TEMPLATE = app
macx: TARGET = $${APP_DISPLAYNAME}
else: TARGET = $${APP_NAME}
CONFIG -= qtquickcompiler
CONFIG += lrelease embed_translations
QT += network qml quick quickcontrols2 multimedia
greaterThan(QT_MAJOR_VERSION,5): QT += core5compat

# The SshChannelExec class requires a private QIODevice header to call setReadChannelCount() for stdout/stderr
QT += core-private

DEFINES += LIBSSH_STATIC SSH_NO_CPP_EXCEPTIONS

android {
##### Not implemented yet! #####
    QT += androidextras
    ANDROID_EXTRA_LIBS += \
        $${PWD}/android/openssl/lib/libcrypto_1_1.so \
        $${PWD}/android/openssl/lib/libssl_1_1.so
    INCLUDEPATH += $${PWD}/android/libssh2/include
    LIBS += -L$${PWD}/android/openssl/lib -lcrypto_1_1 -lssl_1_1 $${PWD}/android/libssh2/lib/libssh2.a
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
} else {
    INCLUDEPATH += \"$${OUT_PWD}/../include\" \"$${OUT_PWD}/../include/freerdp2\" \"$${OUT_PWD}/../include/winpr2\"
    LIBS += -L../lib -lssh -lvncclient
    LIBS += -lfreerdp-client2 ../lib/freerdp2/*.$${QMAKE_EXTENSION_STATICLIB} -lfreerdp2 -lwinpr2
    LIBS += -lcrypto -lssl -lcairo -ljpeg -lpng -llzo2 -lz
    #LIBS += $${QMAKE_LIBS_X11} -lxkbfile
    *bsd: LIBS += -lutil
}

SOURCES += \
    main.cpp \
    DesktopAction.cpp \
    DesktopClient.cpp \
    DesktopView.cpp \
    HttpRequest.cpp \
    KeyLoader.cpp \
    Parser.cpp \
    PtyIFace.cpp \
    RdpAudioPlugin.cpp \
    RdpDesktopClient.cpp \
    SshChannel.cpp \
    SshChannelExec.cpp \
    SshChannelFtp.cpp \
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
    HttpRequest.h \
    KeyLoader.h \
    Parser.h \
    PtyIFace.h \
    RdpAudioPlugin.h \
    RdpDesktopClient.h \
    SshChannel.h \
    SshChannelExec.h \
    SshChannelFtp.h \
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

TRANSLATIONS += $$files(translations/$${APP_NAME}-*.ts)
