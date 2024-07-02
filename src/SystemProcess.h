#ifndef SYSTEMPROCESS_H
#define SYSTEMPROCESS_H

#include <QProcess>

class SystemProcess : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemProcess)

    Q_PROPERTY(QString       command READ command    WRITE setCommand    NOTIFY commandChanged FINAL)
    Q_PROPERTY(QString    stdOutFile READ stdOutFile WRITE setStdOutFile NOTIFY stdOutFileChanged FINAL)
    Q_PROPERTY(QString    stdErrFile READ stdErrFile WRITE setStdErrFile NOTIFY stdErrFileChanged FINAL)
    Q_PROPERTY(QStringList stdOutput READ stdOutput  NOTIFY stdOutputChanged FINAL)
    Q_PROPERTY(QStringList  stdError READ stdError   NOTIFY stdErrorChanged FINAL)
    Q_PROPERTY(bool          running READ running    NOTIFY runningChanged FINAL)
    Q_PROPERTY(QString    nullDevice READ nullDevice CONSTANT FINAL)

public:
    explicit SystemProcess(QObject *parent = nullptr);
    ~SystemProcess();

    QString command() const;
    void setCommand(const QString &cmd);

    QString stdOutFile() const;
    void setStdOutFile(const QString &filename);

    QString stdErrFile() const;
    void setStdErrFile(const QString &filename);

    QStringList stdOutput() const;
    QStringList stdError() const;
    bool running() const;
    static QString nullDevice();

public slots:
    void start();
    void startCommand(const QString &command);
    void stdInput(const QStringList &lines);
    void cancel();
    void clearAll();

signals:
    void commandChanged();
    void stdOutFileChanged();
    void stdErrFileChanged();
    void runningChanged();
    void stdOutputChanged(const QString &text);
    void stdErrorChanged(const QString &text);
    void execError(const QString &text);
    void finished(int code);
    void canceled();

private:
    void setRunning(bool on);
    void onErrorOccurred(QProcess::ProcessError errcode);
    void onFinished(int code, QProcess::ExitStatus status);

    QString run_command;
    QString std_out_file, std_err_file;

    QProcess *run_process;
    bool now_running;
    bool run_canceled;
    QStringList std_output, std_error;
};

#endif // !SYSTEMPROCESS_H
