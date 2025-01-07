#pragma once

#include <QObject>
#include <QVariant>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

class SystemHelper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemHelper)

    Q_PROPERTY(QString   appCacheDir  READ appCacheDir   CONSTANT FINAL)
    Q_PROPERTY(QString   appConfigDir READ appConfigDir  CONSTANT FINAL)
    Q_PROPERTY(QString     appDataDir READ appDataDir    CONSTANT FINAL)
    Q_PROPERTY(QString     appTempDir READ appTempDir    CONSTANT FINAL)
    Q_PROPERTY(QString      appSshKey READ appSshKey     CONSTANT FINAL)
    Q_PROPERTY(QString     appHomeUrl READ appHomeUrl    CONSTANT FINAL)
    Q_PROPERTY(QString     platformOS READ platformOS    CONSTANT FINAL)
    Q_PROPERTY(QString       buildAbi READ buildAbi      CONSTANT FINAL)
    Q_PROPERTY(QString   buildCpuArch READ buildCpuArch  CONSTANT FINAL)
    Q_PROPERTY(QString        cpuArch READ cpuArch       CONSTANT FINAL)
    Q_PROPERTY(int           cpuCores READ cpuCores      CONSTANT FINAL)
    Q_PROPERTY(QString     kernelType READ kernelType    CONSTANT FINAL)
    Q_PROPERTY(QString  kernelVersion READ kernelVersion CONSTANT FINAL)
    Q_PROPERTY(QString       userName READ userName      CONSTANT FINAL)
    Q_PROPERTY(QString       hostName READ hostName      CONSTANT FINAL)
    Q_PROPERTY(QString     domainName READ domainName    CONSTANT FINAL)
    Q_PROPERTY(QString    qtRCVersion READ qtRCVersion   CONSTANT FINAL)
    Q_PROPERTY(QString     sslVersion READ sslVersion    CONSTANT FINAL)
    Q_PROPERTY(QString     sshVersion READ sshVersion    CONSTANT FINAL)
    Q_PROPERTY(QString     rdpVersion READ rdpVersion    CONSTANT FINAL)
    Q_PROPERTY(QString     vncVersion READ vncVersion    CONSTANT FINAL)
    Q_PROPERTY(QString   settingsPath READ settingsPath  CONSTANT FINAL)
    Q_PROPERTY(bool          isMobile READ isMobile      CONSTANT FINAL)

public:
    static constexpr char const *defaultSshKeyName = "id_ed25519"; // ED25519 supported only!

    explicit SystemHelper(QObject *parent = nullptr);
    ~SystemHelper();

    static QString appCacheDir();
    static QString appConfigDir();
    static QString appDataDir();
    static QString appTempDir();
    static QString appSshKey();
    static QString appHomeUrl();
    static QString platformOS();
    static QString buildAbi();
    static QString buildCpuArch();
    static QString cpuArch();
    static int cpuCores();
    static QString kernelType();
    static QString kernelVersion();
    static QString userName();
    static QString hostName();
    static QString domainName();
    static QString qtRCVersion();
    static QString sslVersion();
    static QString sshVersion();
    static QString rdpVersion();
    static QString vncVersion();
    static QString settingsPath();
    static bool isMobile();

    Q_INVOKABLE static QString appCachePath(const QString &name);
    Q_INVOKABLE static QString appConfigPath(const QString &name);
    Q_INVOKABLE static QString appDataPath(const QString &name);
    Q_INVOKABLE static QString appTempPath(const QString &name);
    Q_INVOKABLE static QString envVariable(const QString &name);
    Q_INVOKABLE static QString userHome(const QString &name = QString()); // empty: current user
    Q_INVOKABLE static QStringList groupMembers(const QString &name = QString()); // empty: current user

    Q_INVOKABLE static QStringList sshKeyPairs();
    Q_INVOKABLE static bool isSshKeyPair(const QString &path);
    Q_INVOKABLE static bool isSshPrivateKey(const QString &path);
    Q_INVOKABLE static QString sshPublicKey(const QString &path);

    Q_INVOKABLE static QString pathName(const QString &path = QString()); // empty: current dir
    Q_INVOKABLE static QString fileName(const QString &path = QString()); // empty: temp file
    Q_INVOKABLE static QString baseName(const QString &path);
    Q_INVOKABLE static bool isAbsolute(const QString &path);

    Q_INVOKABLE static bool isDir(const QString &path, bool writable = false);
    Q_INVOKABLE static bool isFile(const QString &path, bool writable = false);
    Q_INVOKABLE static bool isExecutable(const QString &path);
    Q_INVOKABLE static bool removeDir(const QString &path, bool recursively = false);
    Q_INVOKABLE static bool makeDir(const QString &path);
    Q_INVOKABLE static bool removeFile(const QString &name);
    Q_INVOKABLE static bool renameFile(const QString &from, const QString &to);
    Q_INVOKABLE static bool copyFile(const QString &from, const QString &to, bool replace = false);
    Q_INVOKABLE static QString findFile(const QString &name, const QStringList &paths = QStringList());
    Q_INVOKABLE static QString findExecutable(const QString &name, const QStringList &paths = QStringList());
    Q_INVOKABLE static qreal fileSize(const QString &path);
    Q_INVOKABLE static QDateTime fileTime(const QString &path);
    Q_INVOKABLE static QStringList fileList(const QString &path = QString(), // appDataDir by default
                                            const QString &mask = QString(), bool dirsOnly = false);

    Q_INVOKABLE static QVariant loadSettings(const QString &name, const QVariant &defValue = QVariant());
    Q_INVOKABLE static void saveSettings(const QString &name, const QVariant &value = QVariant()); // invalid to remove

    Q_INVOKABLE static QJsonArray loadArray(const QString &path);
    Q_INVOKABLE static QString saveArray(const QString &path, const QJsonArray &json);

    Q_INVOKABLE static QJsonObject loadObject(const QString &path);
    Q_INVOKABLE static QString saveObject(const QString &path, const QJsonObject &json);

    Q_INVOKABLE static QStringList loadText(const QString &path);
    Q_INVOKABLE static QString saveText(const QString &path, const QStringList &text, bool append = false);

    Q_INVOKABLE static void setClipboard(const QString &text); // set null QString() to clear clipboard
    Q_INVOKABLE static QString clipboard();

    Q_INVOKABLE static QString camelCase(const QString &str, QChar sep = ' ');
    Q_INVOKABLE static QString shortcutText(const QVariant &key);

    Q_INVOKABLE static void setCursorShape(int shape = -1); // Qt::CursorShape, -1 to restore

#ifdef Q_OS_ANDROID
    Q_INVOKABLE static bool setAndroidPermission(const QStringList &permissions);
#endif
};
