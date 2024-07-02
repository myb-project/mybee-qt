#ifndef SSHCONNECTION_H
#define SSHCONNECTION_H

#include <QObject>
#include <QUrl>
#include <QScopedPointer>

#include "Ssh2Client.h"
#include "Ssh2Process.h"

class SshConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString       user READ user        WRITE setUser        NOTIFY userChanged FINAL)
    Q_PROPERTY(QString       host READ host        WRITE setHost        NOTIFY hostChanged FINAL)
    Q_PROPERTY(int           port READ port        WRITE setPort        NOTIFY portChanged FINAL)
    Q_PROPERTY(int        timeout READ timeout     WRITE setTimeout     NOTIFY timeoutChanged FINAL)
    Q_PROPERTY(bool   checkServer READ checkServer WRITE setCheckServer NOTIFY checkServerChanged FINAL)
    Q_PROPERTY(QString privateKey READ privateKey  WRITE setPrivateKey  NOTIFY privateKeyChanged FINAL)
    Q_PROPERTY(QString    command READ command     WRITE setCommand     NOTIFY commandChanged FINAL)
    Q_PROPERTY(QString   termType READ termType    WRITE setTermType    NOTIFY termTypeChanged)
    Q_PROPERTY(QStringList setEnv READ setEnv      WRITE setSetEnv      NOTIFY setEnvChanged FINAL)
    Q_PROPERTY(bool      autoExec READ autoExec    WRITE setAutoExec    NOTIFY autoExecChanged FINAL)
    Q_PROPERTY(bool        active READ active      WRITE setActive      NOTIFY activeChanged FINAL)
    Q_PROPERTY(QUrl    connection READ connection  NOTIFY connectionChanged FINAL)
    Q_PROPERTY(bool       running READ running     NOTIFY runningChanged FINAL)
    Q_PROPERTY(QString  lastError READ lastError   NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString exitSignal READ exitSignal  NOTIFY exitSignalChanged FINAL)
    Q_PROPERTY(int     exitStatus READ exitStatus  NOTIFY exitStatusChanged FINAL)

public:
    static constexpr char const *knownHostsName = "known_hosts";

    explicit SshConnection(QObject *parent = nullptr);
    ~SshConnection();

    QString user() const;
    void setUser(const QString &name);

    QString host() const;
    void setHost(const QString &addr);

    int port() const;
    void setPort(int num);

    int timeout() const; // seconds
    void setTimeout(int seconds);

    bool checkServer() const;
    void setCheckServer(bool enable);

    QString privateKey() const;
    void setPrivateKey(const QString &path);

    QString command() const;
    void setCommand(const QString &cmd);

    QString termType() const;
    void setTermType(const QString &term);

    QStringList setEnv() const;
    void setSetEnv(const QStringList &env);

    bool autoExec() const;
    void setAutoExec(bool on);

    bool active() const;
    void setActive(bool on);

    QUrl connection() const;
    bool running() const;

    QString lastError() const;
    QString exitSignal() const;
    int exitStatus() const;

    Q_INVOKABLE bool start(const QUrl &url);
    Q_INVOKABLE bool execute(const QString &cmd = QString()); // empty to [re]run command

public slots:
    void stdInputText(const QString &text);

signals:
    void userChanged();
    void hostChanged();
    void portChanged();
    void timeoutChanged();
    void checkServerChanged();
    void privateKeyChanged();
    void commandChanged();
    void termTypeChanged();
    void setEnvChanged();
    void autoExecChanged();
    void activeChanged();
    void channelsChanged(int channels);
    void openChannelsChanged(int channels);
    void connectionChanged();
    void runningChanged();
    void lastErrorChanged();
    void exitSignalChanged(const QString &name);
    void exitStatusChanged(int code);
    void stdOutputChanged(const QByteArray &data);
    void stdErrorChanged(const QByteArray &data);

private:
    void startSession();
    void runProcess(bool on);
    void onSessionStateChanged(Ssh2Client::SessionStates ssh2_state);
    void onSsh2Error(std::error_code ssh2_error);
    void onNewChannelData(QByteArray data, Ssh2Process::ChannelStream stream_id);

    QString user_name;
    QString host_addr;
    int port_num;
    int timeout_sec;
    bool check_server;
    QString private_key;
    QString exec_cmd;
    QString term_type;
    QStringList set_env;
    bool auto_exec;

    QUrl conn_url;
    bool session_active;
    bool busy_running;
    QString exit_signal;
    int exit_status;

    std::error_code last_error;
    QScopedPointer<Ssh2Client> ssh2_client;
    QPointer<Ssh2Process> ssh2_process;
};

#endif // SSHCONNECTION_H
