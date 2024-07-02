/*
MIT License

Copyright (c) 2023 Vladimir Vorobyev <b800xy@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <QStandardPaths>
#include <QHostInfo>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

#include "Ssh2Settings.h"

static QString getLocalHostname()
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

static QString getSshPrivateKey()
{
    static QString private_key;
    if (private_key.isEmpty()) {
#if defined(__mobile__)
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else
        QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.ssh";
#endif
        private_key = path + "/id_ed25519";
        if (!QFile::exists(private_key))
            private_key = path + "/id_rsa";
    }
    return private_key;
}

static QString getSshKnownHosts()
{
    static QString known_hosts;
    if (known_hosts.isEmpty()) {
#if defined(__mobile__)
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else
        QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.ssh";
#endif
        known_hosts = path + "/known_hosts";
    }
    return known_hosts;
}

class Ssh2SettingsData: public QSharedData
{
public:
    Ssh2SettingsData()
        : user(QCoreApplication::applicationName())
        , host(Ssh2Settings::defaulHost)
        , port(Ssh2Settings::defaultPort)
        , key(getSshPrivateKey())
        , known_hosts(getSshKnownHosts())
        , timeout(Ssh2Settings::defaultTimeout)
        , autoAppendToKnownHosts(QCoreApplication::applicationName() + '@' + getLocalHostname())
        , checkServerIdentity(false)
    {}
    Ssh2SettingsData(const Ssh2SettingsData &other)
        : QSharedData(other)
        , user(other.user)
        , host(other.host)
        , port(other.port)
        , passphrase(other.passphrase)
        , key(other.key)
        , keyphrase(other.keyphrase)
        , known_hosts(other.known_hosts)
        , timeout(other.timeout)
        , autoAppendToKnownHosts(other.autoAppendToKnownHosts)
        , checkServerIdentity(other.checkServerIdentity)
    {}
    ~Ssh2SettingsData() {}

    QString user;
    QString host;
    quint16 port;
    QString passphrase;
    QString key;
    QString keyphrase;
    QString known_hosts;
    uint timeout;
    QString autoAppendToKnownHosts;
    bool checkServerIdentity;
};

Ssh2Settings::Ssh2Settings()
    : d(new Ssh2SettingsData)
{
}

Ssh2Settings::Ssh2Settings(const QString &user, const QString &host, quint16 port)
    : d(new Ssh2SettingsData)
{
    setUser(user);
    setHost(host);
    setPort(port);
}

Ssh2Settings::Ssh2Settings(const Ssh2Settings &other)
    : d(other.d)
{
}

Ssh2Settings &Ssh2Settings::operator=(const Ssh2Settings &other)
{
    if (this != &other) d.operator=(other.d);
    return *this;
}

Ssh2Settings::~Ssh2Settings()
{
}

bool Ssh2Settings::operator==(const Ssh2Settings &other) const
{
    return (d->user == other.d->user &&
            d->host == other.d->host &&
            d->port == other.d->port &&
            d->passphrase == other.d->passphrase &&
            d->key == other.d->key &&
            d->keyphrase == other.d->keyphrase &&
            d->known_hosts == other.d->known_hosts &&
            d->timeout == other.d->timeout &&
            d->autoAppendToKnownHosts == other.d->autoAppendToKnownHosts &&
            d->checkServerIdentity == other.d->checkServerIdentity);
}

bool Ssh2Settings::operator!=(const Ssh2Settings &other) const
{
    return (d->user != other.d->user ||
            d->host != other.d->host ||
            d->port != other.d->port ||
            d->passphrase != other.d->passphrase ||
            d->key != other.d->key ||
            d->keyphrase != other.d->keyphrase ||
            d->known_hosts != other.d->known_hosts ||
            d->timeout != other.d->timeout ||
            d->autoAppendToKnownHosts != other.d->autoAppendToKnownHosts ||
            d->checkServerIdentity != other.d->checkServerIdentity);
}

bool Ssh2Settings::isValid() const
{
    return !d->host.isEmpty();
}

bool Ssh2Settings::isPasswordAuth() const
{
    return !d->passphrase.isEmpty();
}

bool Ssh2Settings::isKeyAuth() const
{
    return !isPasswordAuth();
}

QString Ssh2Settings::user() const
{
    return d->user;
}

QString Ssh2Settings::host() const
{
    return d->host;
}

quint16 Ssh2Settings::port() const
{
    return d->port;
}

QString Ssh2Settings::passPhrase() const
{
    return d->passphrase;
}

QString Ssh2Settings::key() const
{
    return d->key;
}

QString Ssh2Settings::keyPhrase() const
{
    return d->keyphrase;
}

QString Ssh2Settings::knownHosts() const
{
    return d->known_hosts;
}

uint Ssh2Settings::timeout() const
{
    return d->timeout;
}

QString Ssh2Settings::autoAppendToKnownHosts() const
{
    return d->autoAppendToKnownHosts;
}

bool Ssh2Settings::checkServerIdentity() const
{
    return d->checkServerIdentity;
}

void Ssh2Settings::setUser(const QString &value)
{
    d->user = value.isEmpty() ? QCoreApplication::applicationName() : value;
}

void Ssh2Settings::setHost(const QString &value)
{
    d->host = !value.isEmpty() ? value : defaulHost;
}

void Ssh2Settings::setPort(quint16 value)
{
    d->port = value ? value : defaultPort;
}

void Ssh2Settings::setPassPhrase(const QString &value)
{
    d->passphrase = value;
}

void Ssh2Settings::setKey(const QString &value)
{
    d->key = value.isEmpty() ? getSshPrivateKey() : value;
}

void Ssh2Settings::setKeyPhrase(const QString &value)
{
    d->keyphrase = value;
}

void Ssh2Settings::setKnownHosts(const QString &value)
{
    d->known_hosts = value.isEmpty() ? getSshKnownHosts() : value;
}

void Ssh2Settings::setTimeout(uint value)
{
    d->timeout = value ? value : defaultTimeout;
}

void Ssh2Settings::setAutoAppendKnownHost(const QString &value)
{
    d->autoAppendToKnownHosts = value.isEmpty() ? QCoreApplication::applicationName() + '@' + getLocalHostname() : value;
}

void Ssh2Settings::setCheckServerIdentity(bool enable)
{
    d->checkServerIdentity = enable;
}

void Ssh2Settings::dump(QDebug &dbg) const
{
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg << "\nSsh2Settings:"
        << "\n\t user"                   << d->user
        << "\n\t host"                   << d->host
        << "\n\t port"                   << d->port
        << "\n\t passphrase"             << d->passphrase
        << "\n\t key"                    << d->key
        << "\n\t keyphrase"              << d->keyphrase
        << "\n\t known_hosts"            << d->known_hosts
        << "\n\t timeout"                << d->timeout
        << "\n\t autoAppendToKnownHosts" << d->autoAppendToKnownHosts
        << "\n\t checkServerIdentity"     << d->checkServerIdentity;
}
