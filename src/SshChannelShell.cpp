#include <QTimer>
#include <QtDebug>

#include "SshChannelShell.h"
#include "SshSession.h"

//#define TRACE_SSHCHANNELSHELL
#ifdef  TRACE_SSHCHANNELSHELL
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshChannelShell::SshChannelShell(ssh::Session &sshSession, const QString &termType, const QStringList &envVars)
    : SshChannel(sshSession)
    , term_type(termType)
    , env_vars(envVars)
    , later_timer(nullptr)
{
    TRACE_ARG(term_type << env_vars);
    setCallbacks();
    connect(this, &SshChannel::readyRead, this, [this]() {
        emit textReceived(QString::fromUtf8(readAll()));
    });
    callLater(&SshChannelShell::libOpenSession);
}

SshChannelShell::~SshChannelShell()
{
    TRACE();
}

void SshChannelShell::callLater(libShellFunc func, int msec)
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

void SshChannelShell::libOpenSession()
{
    TRACE();
    int rc = ::ssh_channel_open_session(lib_channel.getCChannel());
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelShell::libOpenSession, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_open_session: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    callLater(env_vars.isEmpty() ? &SshChannelShell::libRequestPtySize : &SshChannelShell::libRequestEnv);
}

void SshChannelShell::libRequestEnv()
{
    TRACE();
    QStringList tokens;
    if (!env_vars.isEmpty()) tokens = env_vars.first().split('=', Qt::SkipEmptyParts);
    if (tokens.size() >= 2 && !tokens.at(0).isEmpty()) {
        int rc = ::ssh_channel_request_env(lib_channel.getCChannel(), qPrintable(tokens.at(0)), qPrintable(tokens.at(1)));
        if (rc == SSH_AGAIN) {
            callLater(&SshChannelShell::libRequestEnv, SshSession::callAgainDelay);
            return;
        }
        if (rc != SSH_OK) {
            TRACE_ARG("ssh_channel_request_env:" << ::ssh_get_error(lib_channel.getCSession()));
        }
    }
    callLater(&SshChannelShell::libRequestPtySize);
}

void SshChannelShell::libRequestPtySize()
{
    TRACE();
    int rc = ::ssh_channel_request_pty_size(lib_channel.getCChannel(),
                                            !term_type.isEmpty() ? qPrintable(term_type) : defaultTermType,
                                            defaultTermCols, defaultTermRows);
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelShell::libRequestPtySize, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_request_pty: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    callLater(&SshChannelShell::libRequestShell);
}

void SshChannelShell::libRequestShell()
{
    TRACE();
    int rc = ::ssh_channel_request_shell(lib_channel.getCChannel());
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelShell::libRequestShell, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_request_shell: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    callLater(nullptr);
    if (!QIODevice::open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        emit errorOccurred(QStringLiteral("Open QIODevice failed"));
        return;
    }
    emit channelOpened();
}

void SshChannelShell::sendText(const QString &text)
{
    TRACE_ARG(text);
    if (!isEofSent() && isChannelOpen()) {
        QByteArray data = text.toUtf8();
        if (!data.isEmpty()) write(data);
    }
}
