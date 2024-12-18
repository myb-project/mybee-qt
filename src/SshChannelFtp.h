#ifndef SSHCHANNELFTP_H
#define SSHCHANNELFTP_H

#include <QList>
#include <QPair>

#include "libssh/sftp.h"
#include "SshChannel.h"

class SshChannelFtp;
typedef void (SshChannelFtp::*libFtpFunc)();

class SshChannelFtp : public SshChannel
{
    Q_OBJECT

public:
    explicit SshChannelFtp(ssh::Session &sshSession, const QString &remotePath);
    ~SshChannelFtp() override;

    void pull();
    void push(const QByteArray &data, bool append = false);
    void sendEof() override {}
    void close() override;

signals:
    void channelOpened();

private:
    void callLater(libFtpFunc func, int msec = 0);
    void libOpenSession();
    void libRequestFtp();
    bool libOpenFile(int mode, mode_t perm = 0600);
    void libAsyncReadBegin();
    void libAsyncRead();
    void makeDirPath();
    QString lastError() const;

    QString remote_path;
    sftp_session lib_session;
    QTimer *later_timer;
    sftp_file lib_file;
    sftp_aio lib_aio;
    QStringList mkdir_list;
    QString mkdir_path;
};

#endif // SSHCHANNELFTP_H
