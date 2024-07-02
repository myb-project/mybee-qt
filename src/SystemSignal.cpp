#include <QCoreApplication>
#include <QTimer>
#include <QtDebug>

#include <unistd.h>
#include <errno.h>

#include "SystemSignal.h"

#ifndef _NSIG
#define _NSIG   64
#endif

Q_GLOBAL_STATIC(SystemSignal, globalSystemSignal)

static int supportedSignals[SystemSignal::TotalSignals] = {
    SIGHUP, SIGINT, SIGABRT, SIGBUS, SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGTERM, SIGURG, SIGIO
};

SystemSignal::SystemSignal()
    : QObject(),
      my_uid(::getuid()),
      ignore_uid(false),
      check_pid(0)
{
    sig_queue.reserve(_POSIX_SIGQUEUE_MAX);

    queue_timer = new QTimer(this);
    queue_timer->setInterval(0);
    queue_timer->setSingleShot(true);
    queue_timer->callOnTimeout([this]() {
        while (!sig_queue.isEmpty()) {
            auto pair = sig_queue.dequeue();
            emit recvSignal(pair.first, pair.second);
        }
    });

    if (QCoreApplication::instance())
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &SystemSignal::restoreActions);
}

SystemSignal::~SystemSignal()
{
    restoreActions();
}

void SystemSignal::restoreActions()
{
    queue_timer->stop();
    if (!oldact_map.isEmpty()) {
        for (auto it = oldact_map.begin(); it != oldact_map.end(); it = oldact_map.erase(it)) {
            ::sigaction(it.key(), &it.value(), 0);
        }
    }
}

// static
SystemSignal *SystemSignal::instance(const QList<int> &set)
{
    auto singleton = globalSystemSignal;
    Q_ASSERT(singleton);
    if (!set.isEmpty()) singleton->setSignalSet(set);
    return singleton;
}

// static
void SystemSignal::setEnabled(bool enable)
{
    if (!globalSystemSignal.exists()) return;

    auto self = globalSystemSignal;
    if (self && !self->sig_set.isEmpty()) {
        sigset_t ss;
        ::sigemptyset(&ss);
        for (int i = 0; i < self->sig_set.size(); i++) {
            ::sigaddset(&ss, self->sig_set.at(i));
        }
        if (::pthread_sigmask(enable ? SIG_UNBLOCK : SIG_BLOCK, &ss, 0) < 0)
            qWarning() << Q_FUNC_INFO << "pthread_sigmask" << ::strerror(errno);
    }
}

void SystemSignal::setIgnoreUid(bool yes)
{
    if (yes != ignore_uid) {
        ignore_uid = yes;
        emit ignoreUidChanged();
    }
}

bool SystemSignal::ignoreUid() const
{
    return ignore_uid;
}

void SystemSignal::setCheckPid(pid_t pid)
{
    if (pid != check_pid) {
        check_pid = pid;
        emit checkPidChanged();
    }
}

pid_t SystemSignal::checkPid() const
{
    return check_pid;
}

static void saHandler(int sig)
{
    if (globalSystemSignal.exists())
        SystemSignal::instance()->parseSignal(sig);
}

static void saSigaction(int sig, siginfo_t *info, void *)
{
    if (globalSystemSignal.exists())
        SystemSignal::instance()->parseSignal(sig, info);
}

void SystemSignal::setSignalSet(const QList<int> &set)
{
    if (set == sig_set) return;

    sig_set.clear();
    struct sigaction sa;
    ::memset(&sa, 0, sizeof(struct sigaction));
    ::sigemptyset(&sa.sa_mask);
    if (!set.isEmpty()) {
        if (set.first() == SigAll) {
            for (int i = 0; i < SystemSignal::TotalSignals; i++) {
                int sig = supportedSignals[i];
                if (::sigaddset(&sa.sa_mask, sig) == 0) {
                    if (!oldact_map.contains(sig) && ::sigaction(sig, 0, &sa) == 0)
                        oldact_map[sig] = sa; // backup original signal action
                    sig_set += sig;
                }
            }
        } else for (const auto &sig : set) {
            if (::sigaddset(&sa.sa_mask, sig) == 0) {
                if (!oldact_map.contains(sig) && ::sigaction(sig, 0, &sa) == 0)
                    oldact_map[sig] = sa; // backup original signal action
                sig_set += sig;
            }
        }
    }
    for (auto it = oldact_map.begin(); it != oldact_map.end();) {
        if (!sig_set.contains(it.key())) { // restore original signal action
            ::sigaction(it.key(), &it.value(), 0);
            it = oldact_map.erase(it);
        } else ++it;
    }
    if (!sig_set.isEmpty()) {
        for (int i = 0; i < sig_set.size(); i++) {
            if (sig_set.at(i) != SIGURG) {
                sa.sa_handler = saHandler;
                sa.sa_flags = SA_RESTART;
            } else {
                sa.sa_sigaction = saSigaction;
                sa.sa_flags = SA_RESTART | SA_SIGINFO;
            }
            if (::sigaction(sig_set.at(i), &sa, 0) < 0) {
                qWarning() << Q_FUNC_INFO << "sigaction" << ::strerror(errno);
                return;
            }
        }
        if (::pthread_sigmask(SIG_UNBLOCK, &sa.sa_mask, 0) < 0) {
            qWarning() << Q_FUNC_INFO << "pthread_sigmask" << ::strerror(errno);
            return;
        }
    }
    emit signalSetChanged();
}

QList<int> SystemSignal::signalSet() const
{
    return sig_set;
}

// static
bool SystemSignal::isSupported(int signum)
{
    bool ok = false;
    if (signum > 0 && signum < _NSIG) {
        for (int i = 0; i < SystemSignal::TotalSignals; i++) {
            if (supportedSignals[i] == signum) {
                ok = true;
                break;
            }
        }
    }
    return (ok && ::sigaction(signum, 0, 0) == 0);
}

// static
bool SystemSignal::sendSignal(int pid, SignalNumber sig, int val)
{
    if (!pid) return false;

    if (sig == SIGURG) {
        sigval sv;
        sv.sival_int = val;
        if (::sigqueue(pid, sig, sv) < 0) {
            qWarning() << Q_FUNC_INFO << "sigqueue" << ::strerror(errno);
            return false;
        }
    } else if (::kill(pid, sig) < 0) {
        qWarning() << Q_FUNC_INFO << "kill" << ::strerror(errno);
        return false;
    }
    return true;
}

void SystemSignal::parseSignal(int sig, const siginfo_t *info)
{
    if (!sig_set.contains(sig)) {
        qWarning() << Q_FUNC_INFO << "Unexpected signal" << sig;
        return;
    }
    int signum = sig;
    int sigval = 0;
    if (info && info->si_code == SI_QUEUE) {
        if ((!ignore_uid && info->si_uid != my_uid) || (check_pid && info->si_pid != check_pid))
            return;
        signum = info->si_signo;
#ifdef Q_OS_FREEBSD
        sigval = info->si_mqd;
#else
        sigval = info->si_int;
#endif
    }
    int i = 0;
    for (; i < sig_queue.size(); i++) {
        if (sig_queue[i].first == signum) {
            sig_queue[i].second = sigval;
            break;
        }
    }
    if (i == sig_queue.size() && sig_queue.size() < _POSIX_SIGQUEUE_MAX)
        sig_queue.enqueue(qMakePair(signum, sigval));

    if (!sig_queue.isEmpty() && !queue_timer->isActive())
        queue_timer->start();
}
