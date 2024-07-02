#include <QCoreApplication>
#include <QByteArray>

#include "SshConnection.h"

using namespace qlibssh2;

int main(int argc, char *argv[])
{
    QCoreApplication app{argc, argv};

    SshConnection ssh;
    QObject::connect(&ssh, &SshConnection::stdOutputChanged, &app, [](const QByteArray &data) {
        qDebug() << "stdout" << QString::fromUtf8(data);
    });
    QObject::connect(&ssh, &SshConnection::stdErrorChanged, &app, [](const QByteArray &data) {
        qDebug() << "stderror" << QString::fromUtf8(data);
    });
    QObject::connect(&ssh, &SshConnection::exitStatusChanged, &app, [](int code) {
        qDebug() << "exitStatus" << code;
    });
    QObject::connect(&ssh, &SshConnection::exitSignalChanged, &app, [](const QString &name) {
        qDebug() << "exitSignal" << name;
    });
    ssh.start(QUrl("ssh://aspire/id"));

    return app.exec();
}
