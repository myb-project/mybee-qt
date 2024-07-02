#include <QSslSocket>

#include "UrlModel.h"

// static
bool UrlModel::isEmptyAt(const QUrl &url)
{
    return url.isEmpty();
}

// static
bool UrlModel::isValidAt(const QUrl &url)
{
    return url.isValid();
}

// static
bool UrlModel::isLocalAt(const QUrl &url)
{
    return (url.isValid() && url.isLocalFile());
}

// static
bool UrlModel::isRemoteAt(const QUrl &url)
{
    if (!url.isValid() || url.scheme().isEmpty() || url.isLocalFile()) return false;
#ifndef QT_NO_SSL
    return (!url.scheme().endsWith('s') || QSslSocket::supportsSsl());
#else
    return true;
#endif
}

// static
QString UrlModel::schemeAt(const QUrl &url)
{
    return url.scheme();
}

// static
QString UrlModel::authorityAt(const QUrl &url)
{
    return url.authority();
}

// static
QString UrlModel::userNameAt(const QUrl &url)
{
    return url.userName();
}

// static
QString UrlModel::passwordAt(const QUrl &url)
{
    return url.password();
}

// static
QString UrlModel::hostAt(const QUrl &url)
{
    return url.host();
}

// static
int UrlModel::portAt(const QUrl &url)
{
    return url.port(0);
}

// static
QString UrlModel::pathAt(const QUrl &url)
{
    QString str = url.path();
    if (!str.isEmpty()) {
        while (str.endsWith('/')) str.truncate(str.length() - 1);
        if (!str.startsWith('/')) str.prepend('/');
    }
    return str;
}

// static
QString UrlModel::hostPathAt(const QUrl &url)
{
    if (isRemoteAt(url)) return url.host() + url.path();
    if (isLocalAt(url)) return url.path();
    return QString();
}

// static
QString UrlModel::queryAt(const QUrl &url)
{
    return url.query();
}

// static
QString UrlModel::fragmentAt(const QUrl &url)
{
    return url.fragment();
}

// static
QString UrlModel::pathNameAt(const QUrl &url)
{
    return pathAt(url.adjusted(QUrl::RemoveFilename));
}

// static
QString UrlModel::fileNameAt(const QUrl &url)
{
    return url.fileName();
}

// static
QString UrlModel::textAt(const QUrl &url)
{
    return url.toDisplayString();
}

// static
QUrl UrlModel::adjustedAt(const QUrl &url, FormatingOptions opt)
{
    return url.adjusted((QUrl::FormattingOptions)opt);
}

UrlModel::UrlModel(QObject *parent)
    : QObject(parent)
{
}

UrlModel::~UrlModel()
{
}

bool UrlModel::reset(const QUrl &url)
{
    if (url == url_model) return false;
    bool was_empty = isEmpty();
    bool was_valid = isValid();
    bool was_local = isLocal();
    bool was_remote = isRemote();
    url_model = url;
    if (was_empty != isEmpty()) emit emptyChanged();
    if (was_valid != isValid()) emit validChanged();
    if (was_local != isLocal()) emit localChanged();
    if (was_remote != isRemote()) emit remoteChanged();
    emit locationChanged();
    return true;
}

void UrlModel::setLocation(const QUrl &url)
{
    QUrl prev_url(url_model);
    if (reset(url)) {
        if (url_model.scheme() != prev_url.scheme())     emit schemeChanged();
        if (url_model.userName() != prev_url.userName()) emit userNameChanged();
        if (url_model.password() != prev_url.password()) emit passwordChanged();
        if (url_model.host() != prev_url.host())         emit hostChanged();
        if (url_model.port(0) != prev_url.port(0))       emit portChanged();
        if (url_model.path() != prev_url.path())         emit pathChanged();
        if (url_model.query() != prev_url.query())       emit queryChanged();
        if (url_model.fragment() != prev_url.fragment()) emit fragmentChanged();
        if (url_model.toDisplayString() != prev_url.toDisplayString()) emit textChanged();
    }
}

void UrlModel::setScheme(const QString &str)
{
    QUrl url(url_model);
    url.setScheme(str);
    if (reset(url)) emit schemeChanged();
}

void UrlModel::setUserName(const QString &str)
{
    QUrl url;
    if (!str.isEmpty()) {
        url = url_model;
        url.setUserName(str);
    } else url = url_model.adjusted(QUrl::RemoveUserInfo);
    if (reset(url)) emit userNameChanged();
}

void UrlModel::setPassword(const QString &str)
{
    QUrl url(url_model);
    url.setPassword(str);
    if (reset(url)) emit passwordChanged();
}

void UrlModel::setHost(const QString &str)
{
    QUrl url(url_model);
    url.setHost(str);
    if (reset(url)) emit hostChanged();
}

void UrlModel::setPort(int num)
{
    QUrl url(url_model);
    int p = qBound(0, num, 65535);
    url.setPort(p ? p : -1);
    if (reset(url)) emit portChanged();
}

void UrlModel::setPath(const QString &str)
{
    QUrl url(url_model);
    url.setPath(str);
    if (reset(url)) emit pathChanged();
}

void UrlModel::setQuery(const QString &str)
{
    QUrl url(url_model);
    url.setQuery(str);
    if (reset(url)) emit queryChanged();
}

void UrlModel::setFragment(const QString &str)
{
    QUrl url(url_model);
    url.setFragment(str);
    if (reset(url)) emit fragmentChanged();
}
