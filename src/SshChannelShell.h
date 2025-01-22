#ifndef SSHCHANNELSHELL_H
#define SSHCHANNELSHELL_H

#include <QSize>

#include "SshChannel.h"

class SshChannelShell;
typedef void (SshChannelShell::*libShellFunc)();

class SshChannelShell : public SshChannel
{
    Q_OBJECT

public:
    static constexpr char const *defaultTermType = "xterm";
    static constexpr int const defaultTermCols   = 80;
    static constexpr int const defaultTermRows   = 24;

    explicit SshChannelShell(ssh::Session &sshSession, const QString &termType, const QSize &termSize,
                             const QStringList &envVars = QStringList());
    ~SshChannelShell() override;

    void sendText(const QString &text);
    void setTermSize(int cols, int rows);

signals:
    void channelOpened();
    void textReceived(const QString &text);

private:
    void callLater(libShellFunc func, int msec = 0);
    void libOpenSession();
    void libRequestEnv();
    void libRequestPtySize();
    void libRequestShell();
    void libChangePtySize();

    QString term_type;
    QSize term_size;
    bool shell_ready;
    QStringList env_vars;
    QTimer *later_timer;
};

#endif // SSHCHANNELSHELL_H
