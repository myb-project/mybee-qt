#ifndef SYSTEMPROCESS_H
#define SYSTEMPROCESS_H

#include <QProcess>
#include <QProcessEnvironment>

class SystemProcess : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemProcess)

    Q_PROPERTY(QString       command READ command    WRITE setCommand    NOTIFY commandChanged FINAL)
    Q_PROPERTY(QStringList   envList READ envList    WRITE setEnvList    NOTIFY envListChanged)
    Q_PROPERTY(QString    stdOutFile READ stdOutFile WRITE setStdOutFile NOTIFY stdOutFileChanged FINAL)
    Q_PROPERTY(QString    stdErrFile READ stdErrFile WRITE setStdErrFile NOTIFY stdErrFileChanged FINAL)
    Q_PROPERTY(int             state READ state      NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool          running READ running    NOTIFY runningChanged FINAL)
    Q_PROPERTY(QStringList stdOutput READ stdOutput  NOTIFY stdOutputChanged FINAL)
    Q_PROPERTY(QStringList  stdError READ stdError   NOTIFY stdErrorChanged FINAL)
    Q_PROPERTY(QString    nullDevice READ nullDevice CONSTANT FINAL)

public:
    explicit SystemProcess(QObject *parent = nullptr);
    ~SystemProcess();

    enum State {
        StateIdle,    // The process is not running
        StateStarting,// The process is starting, but the program has not yet been invoked
        StateRunning, // The process is running and is ready for reading and writing
        StateFailed,  // The process failed to start, i.e. the program is missing or no permissions
        StateCrashed, // The process crashed some time after starting successfully
        StateIOError, // An error while read/write to the process. For example, the process may not be running, or it may have closed its input channel
        StateUnknown  // An unknown error occurred. This is the default return value of error()
    };
    Q_ENUM(State)

    QString command() const;
    void setCommand(const QString &cmd);

    QStringList envList() const;
    void setEnvList(const QStringList &env_list);
    Q_INVOKABLE static QStringList sysEnvList();

    QString stdOutFile() const;
    void setStdOutFile(const QString &filename);

    QString stdErrFile() const;
    void setStdErrFile(const QString &filename);

    int state() const; // return State as int
    bool running() const;
    QStringList stdOutput() const;
    QStringList stdError() const;
    static QString nullDevice();

public slots:
    void start();
    void startCommand(const QString &command);
    void stdInput(const QStringList &lines);
    void stdAppend(const QString &text);
    void cancel();

signals:
    void commandChanged();
    void envListChanged();
    void stdOutFileChanged();
    void stdErrFileChanged();
    void stateChanged();
    void runningChanged();
    void stdOutputChanged(const QString &text);
    void stdErrorChanged(const QString &text);
    void execError(const QString &text);
    void started();
    void finished(int code);
    void canceled();

private:
    void setState(State state);
    void setRunning(bool on);
    void onStateChanged(QProcess::ProcessState new_state);
    void onErrorOccurred(QProcess::ProcessError errcode);
    void onFinished(int code, QProcess::ExitStatus status);

    QString run_command;
    QProcessEnvironment proc_env;
    QString std_out_file, std_err_file;

    QProcess *run_process;
    State process_state;
    bool now_running;
    bool run_canceled;
    QStringList std_output, std_error;
};

#endif // !SYSTEMPROCESS_H
