#include <QDir>
#include <QDirIterator>
#include <QThreadPool>

#include "DirSpaceUsed.h"

DirSpaceUsed::DirSpaceUsed(QObject *parent)
    : QObject(parent)
    , avail_mb(0)
    , free_mb(0)
    , total_mb(0)
    , now_run(0)
{
    setStorage(QString());
}

bool DirSpaceUsed::running() const
{
    return now_run != 0;
}

QString DirSpaceUsed::storage() const
{
    return storage_name;
}

int DirSpaceUsed::availMb() const
{
    return avail_mb;
}

int DirSpaceUsed::freeMb() const
{
    return free_mb;
}

int DirSpaceUsed::totalMb() const
{
    return total_mb;
}

int DirSpaceUsed::start(const QString &dir, bool recursively)
{
    if (dir.isEmpty() || !QDir(dir).exists()) return 0;

    auto task = new DirSpaceUsedTask(dir, recursively, this);
    connect(task, &DirSpaceUsedTask::finished, this, &DirSpaceUsed::finished, Qt::QueuedConnection);
    connect(task, &DirSpaceUsedTask::destroyed, this, &DirSpaceUsed::taskDestroyed);
    QThreadPool::globalInstance()->start(task);

    now_run++;
    emit runningChanged();

    setStorage(dir);
    return task->requestId();
}

void DirSpaceUsed::setStorage(const QString &path)
{
    QString txt = !path.isEmpty() ? path : QDir::homePath();
    if (last_path.isEmpty() || !txt.startsWith(last_path)) {
        last_path = txt;
        storage_info.setPath(last_path);
    }
    if (!storage_info.isValid()) {
        qWarning() << Q_FUNC_INFO << path << "Invalid storage info";
        return;
    }
#ifdef Q_OS_WIN
    txt = storage_info.rootPath();
#else
    txt = QString::fromLatin1(storage_info.device());
    txt += " -> " + storage_info.rootPath();
#endif
    QString name = storage_info.name();
    if (!name.isEmpty()) txt += " \"" + name + '"';

    if (txt != storage_name) {
        storage_name = txt;
        emit storageChanged();
    }
}

void DirSpaceUsed::taskDestroyed()
{
    if (--now_run < 0) now_run = 0;

    if (storage_info.isValid()) {
        storage_info.refresh();

        int mb = 1024 * 1024;
        int amb = storage_info.bytesAvailable() / mb;
        int fmb = storage_info.bytesFree() / mb;
        int tmb = storage_info.bytesTotal() / mb;

        if (amb != avail_mb) {
            avail_mb = amb;
            emit availMbChanged();
        }
        if (fmb != free_mb) {
            free_mb = fmb;
            emit freeMbChanged();
        }
        if (tmb != total_mb) {
            total_mb = tmb;
            emit totalMbChanged();
        }
    }
    emit runningChanged();
}

DirSpaceUsedTask::DirSpaceUsedTask(const QString &dir, bool recursively, QObject *parent)
    : QObject(parent)
    , run_path(dir)
    , run_subdirs(recursively)
{
    static int global_reqid = 0;

    global_reqid++;
    if (!global_reqid) global_reqid++; // skip over 0
    run_reqid = global_reqid;
}

void DirSpaceUsedTask::run()
{
    qreal bytes = 0.0;
    if (!run_path.isEmpty()) {
        QDirIterator it(run_path, run_subdirs ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
             it.next();
             QFileInfo fi = it.fileInfo();
             if (fi.isFile() && !fi.isSymLink())
                 bytes += fi.size();
        }
    }
    emit finished(run_reqid, bytes);
}

int DirSpaceUsedTask::requestId() const
{
    return run_reqid;
}
