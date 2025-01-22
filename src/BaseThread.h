#ifndef BASETHREAD_H
#define BASETHREAD_H

#include <QThread>
#include <QAtomicPointer>

#ifndef Q_OS_WIN
#include "SystemSignal.h"
#endif

template <class T>
class BaseThread : public QThread
{
public:
    // the worker object MUST NOT have a parent!
    explicit BaseThread(T *worker, QObject *parent = nullptr)
        : QThread(parent), _worker(worker) {
        Q_ASSERT(_worker.loadRelaxed());
        _worker.loadRelaxed()->moveToThread(this);
    }
    virtual ~BaseThread() {
        if (_worker.loadAcquire()) {
            quit();
            requestInterruption();
            wait();
        }
    }
    T *worker() const { return _worker.loadRelaxed(); }

protected:
    void run() override {
#ifndef Q_OS_WIN
        SystemSignal::setEnabled(false);
#endif
        QThread::run();
        T *wp = _worker.fetchAndStoreAcquire(nullptr);
        if (wp) delete wp;
    }

private:
    QAtomicPointer<T> _worker;
};

#endif // BASETHREAD_H
