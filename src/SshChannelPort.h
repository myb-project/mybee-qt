#ifndef SSHCHANNELPORT_H
#define SSHCHANNELPORT_H

#include "SshChannel.h"

class SshChannelPort;
typedef void (SshChannelPort::*libPortFunc)();

class SshChannelPort : public SshChannel
{
    Q_OBJECT

public:
    explicit SshChannelPort(ssh::Session &sshSession, const QString &addr, quint16 port);
    ~SshChannelPort() override;

    QString address() const;
    qint16 port() const;

signals:
    void channelOpened();

private:
    void callLater(libPortFunc func, int msec = 0);
    void libOpenForward();

    QString host_addr;
    quint16 port_num;
    QTimer *later_timer;
};

#endif // SSHCHANNELPORT_H
