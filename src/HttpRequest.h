#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QSslConfiguration>
#include <QQueue>
#include <QVariant>
#include <QPointer>
#include <QByteArray>
#include <QUrl>

typedef QPair<QUrl,QVariant> RequestQueueData;

class HttpRequest : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(HttpRequest)

    Q_PROPERTY(QStringList urlSchemes READ urlSchemes    CONSTANT FINAL)
    Q_PROPERTY(bool      sslAvailable READ sslAvailable  CONSTANT FINAL)

    Q_PROPERTY(QUrl     url READ url     NOTIFY urlChanged FINAL)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged FINAL)

public:
    explicit HttpRequest(QObject *parent = nullptr);
    ~HttpRequest();

    QStringList urlSchemes() const;
    static bool sslAvailable();

    QUrl url() const;
    bool running() const;

    Q_INVOKABLE bool sendPost(const QUrl &url, const QJsonObject &json);
    Q_INVOKABLE bool sendGet(const QUrl &url, const QString &key = QString());

public slots:
    void cancel();

signals:
    void urlChanged();
    void runningChanged();
    void recvArray(const QUrl &url, const QJsonArray &json);
    void recvObject(const QUrl &url, const QJsonObject &json);
    void recvError(const QString &text);
    void canceled();

private:
    void setUrl(const QUrl &url);
    void setRunning(bool on);
    bool sendQueue(const RequestQueueData &data);
    void setListenReply(bool on);
    void replyFinished();
    void replyDestroyed(QObject *reply);

    QNetworkAccessManager net_mgr;
    QSslConfiguration ssl_conf;
    QQueue<RequestQueueData> request_queue;
    QUrl request_url;
    bool request_running;

    QPointer<QNetworkReply> last_reply;
    QByteArray reply_data;
};

#endif // HTTPREQUEST_H
