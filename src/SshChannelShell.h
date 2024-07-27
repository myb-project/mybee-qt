#ifndef SSHCHANNELSHELL_H
#define SSHCHANNELSHELL_H

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

    explicit SshChannelShell(ssh::Session &sshSession, const QString &termType, const QStringList &envVars);
    ~SshChannelShell() override;

    void sendText(const QString &text);

signals:
    void channelOpened();
    void textReceived(const QString &text);

private:
    void callLater(libShellFunc func, int msec = 0);
    void libOpenSession();
    void libRequestEnv();
    void libRequestPtySize();
    void libRequestShell();

    QString term_type;
    QStringList env_vars;
    QTimer *later_timer;
};

#endif // SSHCHANNELSHELL_H
