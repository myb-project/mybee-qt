#ifndef SSHPROCESS_H
#define SSHPROCESS_H

#include <QObject>
#include <QUrl>
#include <QFile>
#include <QPointer>
#include <QWeakPointer>

#include "SshSettings.h"

class SshSession;
class SshChannelExec;

class SshProcess : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SshProcess)

    Q_PROPERTY(SshSettings  settings READ settings   WRITE setSettings   NOTIFY settingsChanged FINAL)
    Q_PROPERTY(QString       command READ command    WRITE setCommand    NOTIFY commandChanged FINAL)
    Q_PROPERTY(QString    stdOutFile READ stdOutFile WRITE setStdOutFile NOTIFY stdOutFileChanged FINAL)
    Q_PROPERTY(QString    stdErrFile READ stdErrFile WRITE setStdErrFile NOTIFY stdErrFileChanged FINAL)
    Q_PROPERTY(QStringList stdOutput READ stdOutput  NOTIFY stdOutputChanged FINAL)
    Q_PROPERTY(QStringList  stdError READ stdError   NOTIFY stdErrorChanged FINAL)
    Q_PROPERTY(QUrl              url READ url        NOTIFY urlChanged FINAL)
    Q_PROPERTY(bool          running READ running    NOTIFY runningChanged FINAL)
    Q_PROPERTY(bool        executing READ executing  NOTIFY executingChanged FINAL)
    // The following properties are inherited from the SshSession class
    Q_PROPERTY(bool       shareKey READ shareKey    WRITE setShareKey NOTIFY shareKeyChanged FINAL)
    Q_PROPERTY(QString hostAddress READ hostAddress NOTIFY hostAddressChanged FINAL)
    Q_PROPERTY(QString  pubkeyHash READ pubkeyHash  NOTIFY pubkeyHashChanged FINAL)
    Q_PROPERTY(int       knownHost READ knownHost   NOTIFY knownHostChanged FINAL)

public:
    explicit SshProcess(QObject *parent = nullptr);
    ~SshProcess();

    SshSettings settings() const { return ssh_settings; }
    void setSettings(const SshSettings &settings);

    QString command() const { return run_command; }
    void setCommand(const QString &cmd);

    QString stdOutFile() const { return std_out_file.fileName(); }
    void setStdOutFile(const QString &filename);

    QString stdErrFile() const { return std_err_file.fileName(); }
    void setStdErrFile(const QString &filename);

    QStringList stdOutput() const { return std_output; }
    QStringList stdError() const { return std_error; }
    QUrl url() const { return cmd_url; }
    bool running() const { return now_running; }
    bool executing() const { return now_executing; }

    // The following properties are inherited from the SshSession class
    void setShareKey(bool enable);
    bool shareKey() const;
    QString hostAddress() const;
    QString pubkeyHash() const;
    int knownHost() const;

    Q_INVOKABLE static int defaultPortNumber() { return SshSettings::defaultPortNumber; }
    Q_INVOKABLE static int defaultTimeoutSec() { return SshSettings::defaultTimeoutSec; }
    Q_INVOKABLE static QString defaultTermType() { return SshSettings::defaultTermType; }
    Q_INVOKABLE static QString portNumberKey() { return SshSettings::portNumberKey; }
    Q_INVOKABLE static QString timeoutSecKey() { return SshSettings::timeoutSecKey; }
    Q_INVOKABLE static SshSettings sshSettings(const QString &user, const QString &host,
                                               quint16 port, const QString &key);

public slots:
    void start(const QUrl &url, const QString &key);
    void stdInput(const QStringList &lines);
    void cancel();
    // The following functions are inherited from the SshSession class
    void writeKnownHost();
    void giveAnswers(const QStringList &answers); // must match askQuestions's prompts

signals:
    void settingsChanged();
    void commandChanged();
    void stdOutFileChanged();
    void stdErrFileChanged();
    void urlChanged();
    void runningChanged();
    void executingChanged();
    void stdOutputChanged(const QString &text);
    void stdErrorChanged(const QString &text);
    void execError(const QString &text);
    void finished(int code);
    void canceled();
    // The following signals are inherited from the SshSession class
    void shareKeyChanged();
    void hostAddressChanged();
    void pubkeyHashChanged();
    void knownHostChanged();
    void askQuestions(const QString &info, const QStringList &prompts); // requre giveAnswers()

private:
    void setRunning(bool on);
    void setExecuting(bool on);
    void onStateChanged();
    void onChannelOpened();
    void onChannelClosed();
    void onLastErrorChanged();
    void readStdoutData(const QByteArray &data);
    void readStderrData(const QByteArray &data);

    SshSettings ssh_settings;
    QString run_command;
    QFile std_out_file, std_err_file;
    QUrl cmd_url;

    QPointer<SshSession> ssh_session;
    QWeakPointer<SshChannelExec> ssh_exec;
    bool now_running;
    bool now_executing;    
    QStringList std_output, std_error;
    int exit_status;
};

#endif // !SSHPROCESS_H
