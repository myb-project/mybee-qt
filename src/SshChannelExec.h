#ifndef SSHCHANNELEXEC_H
#define SSHCHANNELEXEC_H

#include "SshChannel.h"

class SshChannelExec;
typedef void (SshChannelExec::*libExecFunc)();

class SshChannelExec : public SshChannel
{
    Q_OBJECT

public:
    explicit SshChannelExec(ssh::Session &sshSession, const QString &command);
    ~SshChannelExec() override;

signals:
    void channelOpened();
    void exitStatusChanged(int exit_status);

protected:
    // reimplemented from SshChannel
    void exitStatusReceived(int exit_status) override;

private:
    void callLater(libExecFunc func, int msec = 0);
    void libOpenSession();
    void libRequestExec();

    QString run_cmd;
    QTimer *later_timer;
};

#endif // SSHCHANNELEXEC_H
