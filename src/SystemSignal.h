#ifndef SYSTEMSIGNAL_H
#define SYSTEMSIGNAL_H

#include <QObject>
#include <QMap>
#include <QQueue>

#include <signal.h>

class QTimer;

class SystemSignal : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemSignal)
    Q_PROPERTY(bool ignoreUid READ ignoreUid WRITE setIgnoreUid NOTIFY ignoreUidChanged)
    Q_PROPERTY(int  checkPid  READ checkPid  WRITE setCheckPid  NOTIFY checkPidChanged)
    Q_PROPERTY(QList<int> signalSet READ signalSet WRITE setSignalSet NOTIFY signalSetChanged)

public:
    enum SignalNumber {
        SigAll  = 0,       // catch all following signals (just for debug signal handlers)
        SigHup  = SIGHUP,  // Term: Hangup on controlling terminal or death of controlling process
        SigInt  = SIGINT,  // Term: Interrupt from keyboard
        SigAbrt = SIGABRT, // Core: Abort signal from abort(3)
        SigBus  = SIGBUS,  // Core: Bus error (bad memory access)
        SigUsr1 = SIGUSR1, // Term: User-defined signal 1
        SigSegv = SIGSEGV, // Core: Invalid memory reference
        SigUsr2 = SIGUSR2, // Term: User-defined signal 2
        SigPipe = SIGPIPE, // Term: Broken pipe. Qt ignore it every time when write data
        SigTerm = SIGTERM, // Term: Termination signal
        SigUrg  = SIGURG,  // Ign: Urgent condition on integer as argument with using ::sigqueue(3)
        SigPoll = SIGIO,   // Term: Pollable event (SysV), synonym for SIGPOLL
        TotalSignals = 11
    };
    Q_ENUM(SignalNumber)

    explicit SystemSignal();
    ~SystemSignal();

    static SystemSignal *instance(const QList<int> &set = QList<int>());
    static void setEnabled(bool enable);

    void setIgnoreUid(bool yes);
    bool ignoreUid() const;

    void setCheckPid(pid_t pid);
    pid_t checkPid() const;

    void setSignalSet(const QList<int> &set);
    QList<int> signalSet() const;

    Q_INVOKABLE static bool isSupported(int signalNumber);
    Q_INVOKABLE static bool sendSignal(int processId, SystemSignal::SignalNumber signalNumber,
                                       int signalValue = 0);

    void parseSignal(int sig, const siginfo_t *info = nullptr);

signals:
    void ignoreUidChanged();
    void checkPidChanged();
    void signalSetChanged();
    void recvSignal(int signalNumber, int signalValue);

private:
    void restoreActions();

    uint my_uid;
    bool ignore_uid;
    pid_t check_pid;
    QList<int> sig_set;
    QMap<int, struct sigaction> oldact_map;
    QQueue<QPair<int, int>> sig_queue;
    QTimer *queue_timer;
};

#endif // SYSTEMSIGNAL_H
