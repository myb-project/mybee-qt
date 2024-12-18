#include <QTimer>
#include <QtDebug>

// Qt private includes
#include <private/qiodevice_p.h>

#include "SshChannelPort.h"
#include "SshSession.h"

//#define TRACE_SSHCHANNELPORT
#ifdef  TRACE_SSHCHANNELPORT
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshChannelPort::SshChannelPort(ssh::Session &sshSession, const QString &addr, quint16 port)
    : SshChannel(sshSession)
    , host_addr(addr)
    , port_num(port)
    , later_timer(nullptr)
{
    TRACE_ARG(host_addr << port_num);
    callLater(&SshChannelPort::libOpenForward);
}

SshChannelPort::~SshChannelPort()
{
    TRACE();
}

QString SshChannelPort::address() const
{
    return host_addr;
}

qint16 SshChannelPort::port() const
{
    return port_num;
}

void SshChannelPort::callLater(libPortFunc func, int msec)
{
    //TRACE_ARG(msec);
    if (!func) {
        if (later_timer) later_timer->stop();
        return;
    }
    if (!later_timer) {
        later_timer = new QTimer(this);
        later_timer->setSingleShot(true);
    } else later_timer->disconnect();
    connect(later_timer, &QTimer::timeout, this, [this,func]() { (this->*func)(); });
    later_timer->start(qBound(0, msec, 1000));
}

void SshChannelPort::libOpenForward()
{
    TRACE();
    int rc = ::ssh_channel_open_forward(lib_channel.getCChannel(),
                                        qPrintable(host_addr), port_num, "localhost", 0);
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelPort::libOpenForward, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_open_forward: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    callLater(nullptr);
    emit channelOpened();
}
