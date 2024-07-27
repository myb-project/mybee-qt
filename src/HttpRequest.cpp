#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QSslError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>

#include "HttpRequest.h"

HttpRequest::HttpRequest(QObject *parent)
    : QObject(parent)
    , request_running(false)
{
#ifndef QT_NO_SSL
    ssl_conf.setDefaultConfiguration(QSslConfiguration::defaultConfiguration());
    ssl_conf.setProtocol(QSsl::TlsV1_2OrLater);
    ssl_conf.setPeerVerifyMode(QSslSocket::VerifyNone);
#endif
}

HttpRequest::~HttpRequest()
{
    if (last_reply) delete last_reply;
}

QStringList HttpRequest::urlSchemes() const
{
    return net_mgr.supportedSchemes();
}

// static
bool HttpRequest::sslAvailable()
{
#ifdef QT_NO_SSL
    return false;
#else
    return (QSslSocket::supportsSsl() &&
            !QSslSocket::sslLibraryBuildVersionString().isEmpty() &&
            !QSslSocket::sslLibraryVersionString().isEmpty());
#endif
}

// static
QString HttpRequest::sslVerCompile()
{
#ifdef QT_NO_SSL
    return QString();
#else
    return QSslSocket::sslLibraryBuildVersionString();
#endif
}

// static
QString HttpRequest::sslVerRuntime()
{
#ifdef QT_NO_SSL
    return QString();
#else
    return QSslSocket::sslLibraryVersionString();
#endif
}

QUrl HttpRequest::url() const
{
    return request_url;
}

void HttpRequest::setUrl(const QUrl &url)
{
    if (url != request_url) {
        request_url = url;
        emit urlChanged();
    }
}

bool HttpRequest::running() const
{
    return request_running;
}

void HttpRequest::setRunning(bool on)
{
    if (on != request_running) {
        request_running = on;
        emit runningChanged();
    }
}

bool HttpRequest::sendPost(const QUrl &url, const QJsonObject &json)
{
    //qDebug() << Q_FUNC_INFO << url << json;

    if (!url.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid URL specified" << url;
        return false;
    }
    bool ssl = url.scheme().toLower().endsWith('s');
    if (ssl && !sslAvailable()) {
        qWarning() << Q_FUNC_INFO << "No SSL support for" << url.scheme();
        return false;
    }
    if (json.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Empty JsonObject specified";
        return false;
    }
    if (last_reply) {
        request_queue.enqueue(qMakePair(url, QVariant(json)));
        return true;
    }

    QNetworkRequest req(url);
    if (ssl) req.setSslConfiguration(ssl_conf);

    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    last_reply = net_mgr.post(req, QJsonDocument(json).toJson());

    setUrl(url);
    setListenReply(true);

    return true;
}

bool HttpRequest::sendGet(const QUrl &url, const QString &key)
{
    if (!url.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid URL specified" << url;
        return false;
    }
    bool ssl = url.scheme().toLower().endsWith('s');
    if (ssl && !sslAvailable()) {
        qWarning() << Q_FUNC_INFO << "No SSL support for" << url.scheme();
        return false;
    }
    if (last_reply) {
        request_queue.enqueue(qMakePair(url, QVariant(key)));
        return true;
    }

    QNetworkRequest req(url);
    if (ssl) req.setSslConfiguration(ssl_conf);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    if (!key.isEmpty()) {
        auto hex = QCryptographicHash::hash(key.toLatin1(), QCryptographicHash::Md5).toHex();
        req.setRawHeader(QByteArray("cid"), hex);
    }
    last_reply = net_mgr.get(req);

    setUrl(url);
    setListenReply(true);

    return true;
}

bool HttpRequest::sendQueue(const RequestQueueData &data)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QMetaType::Type type = (QMetaType::Type)data.second.type();
#else
    QMetaType::Type type = (QMetaType::Type)data.second.typeId();
#endif
    if (type == QMetaType::QJsonObject)
        return sendPost(data.first, data.second.toJsonObject());

    if (type == QMetaType::QString)
        return sendGet(data.first, data.second.toString());

    qWarning() << Q_FUNC_INFO << "Unexpected RequestQueueData type";
    return false;
}

void HttpRequest::setListenReply(bool on)
{
    reply_data.clear();

    if (last_reply) {
        if (on) {
            connect(last_reply, &QIODevice::readyRead, this, [this]() {
                reply_data.append(last_reply->readAll());
            });
            connect(last_reply, &QNetworkReply::finished, this, &HttpRequest::replyFinished);
            connect(last_reply, &QNetworkReply::destroyed, this, &HttpRequest::replyDestroyed);
#ifndef QT_NO_SSL
            connect(last_reply, &QNetworkReply::sslErrors, this, [this](const QList<QSslError> &errors) {
                last_reply->ignoreSslErrors(errors);
            });
#endif
        } else {
            last_reply->disconnect(this);
            last_reply->abort();
            last_reply->deleteLater();
        }
    } else on = false;

    setRunning(on);
}

void HttpRequest::replyFinished()
{
    Q_ASSERT(last_reply);

    last_reply->deleteLater();

    if (last_reply->error() != QNetworkReply::NoError) {
        QVariant code = last_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        int status = code.isValid() ? code.toInt() : last_reply->error();
        emit recvError(QString("%1: %2").arg(status).arg(last_reply->errorString()));
        return;
    }
    if (reply_data.size()) {
        const QJsonDocument jdoc = QJsonDocument::fromJson(reply_data);
        if (jdoc.isObject()) {
            emit recvObject(last_reply->url(), jdoc.object());
            return;
        }
        if (jdoc.isArray()) {
            emit recvArray(last_reply->url(), jdoc.array());
            return;
        }
    }
    emit recvError(tr("Invalid %1 bytes received at %2").arg(reply_data.size()).arg(last_reply->url().toString()));
}

void HttpRequest::replyDestroyed(QObject *reply)
{
    Q_UNUSED(reply);

    if (request_queue.isEmpty() || !sendQueue(request_queue.dequeue())) {
        request_queue.clear();
        setRunning(false);
    }
}

void HttpRequest::cancel()
{
    request_queue.clear();

    setListenReply(false);
    emit canceled();
}
