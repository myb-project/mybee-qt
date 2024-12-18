#include <QTimer>
#include <QtDebug>

// Qt private includes
#include <private/qiodevice_p.h>

#include "SshChannelExec.h"
#include "SshSession.h"

//#define TRACE_SSHCHANNELEXEC
#ifdef  TRACE_SSHCHANNELEXEC
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshChannelExec::SshChannelExec(ssh::Session &sshSession, const QString &command)
    : SshChannel(sshSession)
    , run_cmd(command)
    , later_timer(nullptr)
{
    TRACE_ARG(run_cmd);
    if (run_cmd.isEmpty()) {
        qCritical() << Q_FUNC_INFO << "Empty command specified";
        return;
    }
    if (!isOpen()) return;

    auto dp = reinterpret_cast<QIODevicePrivate*>(QIODevice::d_ptr.data());
    if (!dp) {
        qCritical() << Q_FUNC_INFO << "Can't serve stderr channel";
        return;
    }
    dp->setReadChannelCount(2);
    callLater(&SshChannelExec::libOpenSession);
}

SshChannelExec::~SshChannelExec()
{
    TRACE();
}

// reimplemented from SshChannel
void SshChannelExec::exitStatusReceived(int status)
{
    TRACE_ARG(status);
    emit exitStatusChanged(status);
}

void SshChannelExec::callLater(libExecFunc func, int msec)
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

void SshChannelExec::libOpenSession()
{
    TRACE();
    int rc = ::ssh_channel_open_session(lib_channel.getCChannel());
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelExec::libOpenSession, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_open_session: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    callLater(&SshChannelExec::libRequestExec);
}

void SshChannelExec::libRequestExec()
{
    TRACE();
    int rc = ::ssh_channel_request_exec(lib_channel.getCChannel(), qPrintable(run_cmd));
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelExec::libRequestExec, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_request_Exec: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    callLater(nullptr);
    emit channelOpened();
}
