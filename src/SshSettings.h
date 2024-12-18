#ifndef SSHSETTINGS_H
#define SSHSETTINGS_H

#include <QMetaType>
#include <QSharedDataPointer>
#include <QStringList>
//#include <QQmlEngine>

class QUrl;
class SshSettingsData;

class SshSettings
{
    Q_GADGET
    Q_PROPERTY(QString        user READ user        WRITE setUser       )
    Q_PROPERTY(QString        host READ host        WRITE setHost       )
    Q_PROPERTY(int            port READ port        WRITE setPort       )
    Q_PROPERTY(int         timeout READ timeout     WRITE setTimeout    ) // in seconds
    Q_PROPERTY(QString    password READ password    WRITE setPassword   )
    Q_PROPERTY(QString  privateKey READ privateKey  WRITE setPrivateKey )
    Q_PROPERTY(QString    termType READ termType    WRITE setTermType   )
    Q_PROPERTY(QStringList envVars READ envVars     WRITE setEnvVars    )
    //QML_VALUE_TYPE(sshSettings)

public:
    static constexpr int const defaultPortNumber = 22;
    static constexpr int const defaultTimeoutSec = 5;
    static constexpr char const *defaultTermType = "xterm-256color";
    static constexpr char const *portNumberKey   = "sshPortNumber";
    static constexpr char const *timeoutSecKey   = "sshTimeoutSec";

    SshSettings();
    SshSettings(const QString &user, const QString &host, quint16 port, const QString &key = QString());
    SshSettings(const SshSettings &other);
    SshSettings &operator=(const SshSettings &other);
    ~SshSettings();

    bool operator==(const SshSettings &other) const;
    bool operator!=(const SshSettings &other) const;

    static SshSettings fromUrl(const QUrl &url, const QString &key = QString());
    Q_INVOKABLE bool isValid() const;
    Q_INVOKABLE QString toString(const QString &scheme = QString()) const; // ssh scheme by default

    QString user() const;
    void setUser(const QString &name);

    QString host() const;
    void setHost(const QString &addr);

    int port() const;
    void setPort(int num);
    static bool isPort(int num);

    int timeout() const; // in seconds
    void setTimeout(int seconds);

    QString password() const;
    void setPassword(const QString &pswd);

    QString privateKey() const;
    void setPrivateKey(const QString &path);

    QString termType() const;
    void setTermType(const QString &term);

    QStringList envVars() const;
    void setEnvVars(const QStringList &vars);

    friend inline QDebug& operator<<(QDebug &dbg, const SshSettings &from) {
        from.dump(dbg); return dbg; }

private:
    void dump(QDebug &dbg) const;

    QSharedDataPointer<SshSettingsData> d;
};

Q_DECLARE_TYPEINFO(SshSettings, Q_MOVABLE_TYPE);

Q_DECLARE_METATYPE(SshSettings)

#endif // SSHSETTINGS_H
