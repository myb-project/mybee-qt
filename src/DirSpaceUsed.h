#ifndef DIRSPACEUSED_H
#define DIRSPACEUSED_H

#include <QObject>
#include <QRunnable>
#include <QStorageInfo>

class DirSpaceUsed : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    running READ running NOTIFY runningChanged FINAL)
    Q_PROPERTY(QString storage READ storage NOTIFY storageChanged FINAL)
    Q_PROPERTY(int     availMb READ availMb NOTIFY availMbChanged FINAL)
    Q_PROPERTY(int      freeMb READ freeMb  NOTIFY freeMbChanged FINAL)
    Q_PROPERTY(int     totalMb READ totalMb NOTIFY totalMbChanged FINAL)

public:
    explicit DirSpaceUsed(QObject *parent = nullptr);

    bool running() const;

    QString storage() const;
    int availMb() const;
    int freeMb() const;
    int totalMb() const;

    Q_INVOKABLE int start(const QString &dir, bool recursively = true); // return reqid or 0

signals:
    void runningChanged();
    void storageChanged();
    void availMbChanged();
    void freeMbChanged();
    void totalMbChanged();
    void finished(int reqid, qreal bytes);

private:
    void setStorage(const QString &path);
    void taskDestroyed();

    QStorageInfo storage_info;
    QString last_path;
    QString storage_name;
    int avail_mb;
    int free_mb;
    int total_mb;

    int now_run;
};

class DirSpaceUsedTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit DirSpaceUsedTask(const QString &dir, bool recursively, QObject *parent = nullptr);

    void run() override;
    int requestId() const;

signals:
    void finished(int reqid, qreal bytes);

private:
    QString run_path;
    bool run_subdirs;
    int run_reqid;
};

#endif // DIRSPACEUSED_H
