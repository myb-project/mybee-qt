#include <QSettings>
#include <QHostInfo>
#include <QStandardPaths>
#include <QFile>
#include <QUrl>
#include <QCoreApplication>
#include <QDebug>

#ifdef  Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtAndroidExtras>
#else
#include <QJniEnvironment>
#include <QJniObject>
#endif
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <lmcons.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#include "SshSettings.h"

static QString defaultUserName()
{
    static QString user_name;
    if (user_name.isEmpty()) {
#ifdef  Q_OS_UNIX
#if defined(Q_OS_ANDROID) && __ANDROID_API__ < 28
        char *cp;
#endif
        char buf[4096];
        struct passwd pw, *pwp = nullptr;
        if (::getpwuid_r(::getuid(), &pw, buf, sizeof(buf), &pwp) == 0 && pwp) user_name = pwp->pw_name;
#if defined(Q_OS_ANDROID) && __ANDROID_API__ < 28
        else if ((cp = ::getlogin()) != nullptr) user_name = cp;
#else
        else if (::getlogin_r(buf, sizeof(buf))) user_name = buf;
#endif
        if (user_name.isEmpty()) user_name = qEnvironmentVariable("USER");
#else // Windows?
        char buf[UNLEN + 1];
        DWORD len = UNLEN + 1;
        if (GetUserNameA(buf, &len)) user_name = buf;
        if (user_name.isEmpty()) user_name = qEnvironmentVariable("USERNAME");
#endif
        if (user_name.isEmpty()) user_name = QCoreApplication::applicationName();
    }
    return user_name;
}

static QString defaultHostName()
{
    static QString name;
    if (name.isEmpty()) {
        name = QHostInfo::localHostName();
        if (name.isEmpty()) {
            name = QHostInfo::localDomainName();
            if (name.isEmpty()) name = QStringLiteral("localhost");
        }
    }
    return name;
}

static QString defaultPrivateKey()
{
    static QString private_key;
    if (private_key.isEmpty()) {
#if defined(__mobile__)
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else
        QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.ssh";
#endif
        private_key = path + "/id_ed25519";
        if (!QFile::exists(private_key)) private_key = path + "/id_rsa";
    }
    return private_key;
}

class SshSettingsData : public QSharedData
{
public:
    SshSettingsData()
        : user(defaultUserName())
        , host(defaultHostName())
        , private_key(defaultPrivateKey())
        , term_type(SshSettings::defaultTermType)
    {
        QSettings settings;
        port = settings.value(QLatin1String(SshSettings::portNumberKey), SshSettings::defaultPortNumber).toInt();
        timeout = settings.value(QLatin1String(SshSettings::timeoutSecKey), SshSettings::defaultTimeoutSec).toInt();
    }
    SshSettingsData(const SshSettingsData &other)
        : QSharedData(other)
        , user(other.user)
        , host(other.host)
        , port(other.port)
        , timeout(other.timeout)
        , password(other.password)
        , private_key(other.private_key)
        , term_type(other.term_type)
        , env_vars(other.env_vars)
    {}
    ~SshSettingsData() {}

    QString user;
    QString host;
    quint16 port;
    int timeout;
    QString password; // empty by default
    QString private_key;
    QString term_type;
    QStringList env_vars;
};

SshSettings::SshSettings()
    : d(new SshSettingsData)
{
}

SshSettings::SshSettings(const QUrl &url, const QString &key)
    : d(new SshSettingsData)
{
    if (url.isValid()) {
        if (!url.userName().isEmpty()) setUser(url.userName());
        if (!url.host().isEmpty())     setHost(url.host());
        if (isPort(url.port()))        setPort(url.port());
        if (!key.isEmpty())            setPrivateKey(key);
    }
}

SshSettings::SshSettings(const QString &user, const QString &host, quint16 port, const QString &key)
    : d(new SshSettingsData)
{
    if (!user.isEmpty()) setUser(user);
    if (!host.isEmpty()) setHost(host);
    if (isPort(port))    setPort(port);
    if (!key.isEmpty())  setPrivateKey(key);
}

SshSettings::SshSettings(const SshSettings &other)
    : d(other.d)
{
}

SshSettings &SshSettings::operator=(const SshSettings &other)
{
    if (this != &other) d.operator=(other.d);
    return *this;
}

SshSettings::~SshSettings()
{
}

bool SshSettings::operator==(const SshSettings &other) const
{
    return (d->user == other.d->user &&
            d->host == other.d->host &&
            d->port == other.d->port &&
            d->timeout == other.d->timeout &&
            d->password == other.d->password &&
            d->private_key == other.d->private_key &&
            d->term_type == other.d->term_type &&
            d->env_vars == other.d->env_vars);
}

bool SshSettings::operator!=(const SshSettings &other) const
{
    return (d->user != other.d->user ||
            d->host != other.d->host ||
            d->port != other.d->port ||
            d->timeout != other.d->timeout ||
            d->password != other.d->password ||
            d->private_key != other.d->private_key ||
            d->term_type != other.d->term_type ||
            d->env_vars != other.d->env_vars);
}

bool SshSettings::isValid() const
{
    return (!d->host.isEmpty() && isPort(d->port) && (!d->password.isEmpty() || !d->private_key.isEmpty()));
}

QString SshSettings::toString(const QString &scheme) const
{
    if (!isValid()) return QString();

    QString url = !scheme.isEmpty() ? scheme : "ssh";
    url += "://";

    if (!d->user.isEmpty()) url += d->user + '@';

    url += !d->host.isEmpty() ? d->host : defaultHostName();

    if (isPort(d->port) && d->port != defaultPortNumber) url += ':' + QString::number(d->port);

    return url;
}

QString SshSettings::user() const
{
    return d->user;
}

void SshSettings::setUser(const QString &name)
{
    d->user = name;
}

QString SshSettings::host() const
{
    return d->host;
}

void SshSettings::setHost(const QString &addr)
{
    d->host = !addr.isEmpty() ? addr : defaultHostName();
}

int SshSettings::port() const
{
    return d->port;
}

void SshSettings::setPort(int num)
{
    if (num > 0) d->port = qBound(defaultPortNumber, num, 65535);
}

// static
bool SshSettings::isPort(int num)
{
    return (num > 0 && num <= 65535);
}

int SshSettings::timeout() const
{
    return d->timeout;
}

void SshSettings::setTimeout(int seconds)
{
    if (seconds > 0) d->timeout = qBound(defaultTimeoutSec, seconds, defaultTimeoutSec * 3);
}

QString SshSettings::password() const
{
    return d->password;
}

void SshSettings::setPassword(const QString &pswd)
{
    d->password = pswd;
}

QString SshSettings::privateKey() const
{
    return d->private_key;
}

void SshSettings::setPrivateKey(const QString &path)
{
    d->private_key = (!path.isEmpty() && QFile::exists(path)) ? path : defaultPrivateKey();
}

QString SshSettings::termType() const
{
    return d->term_type;
}

void SshSettings::setTermType(const QString &term)
{
    d->term_type = !term.isEmpty() ? term : defaultTermType;
}

QStringList SshSettings::envVars() const
{
    return d->env_vars;
}

void SshSettings::setEnvVars(const QStringList &vars)
{
    d->env_vars = vars;
}

void SshSettings::dump(QDebug &dbg) const
{
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg << "\nSshSettings:"
        << "\n\t user"          << d->user
        << "\n\t host"          << d->host
        << "\n\t port"          << d->port
        << "\n\t timeout"       << d->timeout
        << "\n\t password"      << d->password
        << "\n\t privateKey"    << d->private_key
        << "\n\t termType"      << d->term_type
        << "\n\t envVars"       << d->env_vars;
}
