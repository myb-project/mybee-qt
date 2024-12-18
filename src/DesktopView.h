#ifndef DESKTOPVIEW_H
#define DESKTOPVIEW_H

#include <QQuickPaintedItem>
#include <QTransform>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QRectF>
#include <QCoreApplication>
#include <QPointer>
#include <QWeakPointer>

#include "RdpDesktopClient.h"
#include "VncDesktopClient.h"
#include "BaseThread.h"

class SshSession;
class SshChannelPort;

class DesktopView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(bool       logging READ logging    WRITE setLogging    NOTIFY loggingChanged FINAL)
    Q_PROPERTY(int        quality READ quality    WRITE setQuality    NOTIFY qualityChanged FINAL)
    Q_PROPERTY(QSize      maxSize READ maxSize    WRITE setMaxSize    NOTIFY maxSizeChanged FINAL)
    Q_PROPERTY(QString       bell READ bell       WRITE setBell       NOTIFY bellChanged FINAL)
    Q_PROPERTY(qreal    viewScale READ viewScale  WRITE setViewScale  NOTIFY viewScaleChanged FINAL)
    Q_PROPERTY(QPointF viewCenter READ viewCenter WRITE setViewCenter NOTIFY viewCenterChanged FINAL)
    Q_PROPERTY(QRectF    viewRect READ viewRect   NOTIFY viewRectChanged FINAL)
    Q_PROPERTY(bool         alive READ alive      NOTIFY aliveChanged FINAL)
    Q_PROPERTY(QSizeF   dimension READ dimension  NOTIFY dimensionChanged FINAL)
    Q_PROPERTY(int         frames READ frames     NOTIFY framesChanged FINAL)
    Q_PROPERTY(QString      error READ error      NOTIFY errorChanged FINAL)

public:
    static constexpr int const mouseMoveJitter = 3; // remote pixels

    explicit DesktopView(QQuickItem *parent = nullptr);
    ~DesktopView();

    enum Quality { // just for translate to Qml
        QualityFast = DesktopClient::QualityFast,
        QualityAve  = DesktopClient::QualityAve, // by default
        QualityBest = DesktopClient::QualityBest
    };
    Q_ENUM(Quality)

    bool logging() const;
    void setLogging(bool enable);

    int quality() const;
    void setQuality(int quality);

    QSize maxSize() const;
    void setMaxSize(const QSize &size);

    QString bell() const;
    void setBell(const QString &wavFile);

    bool alive() const;
    int frames() const;
    QString error() const;
    QSizeF dimension() const;
    QRectF viewRect() const;

    qreal viewScale() const;
    void setViewScale(qreal scale);

    QPointF viewCenter() const;
    void setViewCenter(const QPointF &center);

    Q_INVOKABLE bool setSshTunnel(QObject *ssh, const QUrl &url, const QString &key,
                                  const QString &addr, int port);
    Q_INVOKABLE bool start(const QUrl &url); // desktop url

    // Reimplemented from QQuickPaintedItem
    void paint(QPainter *painter) override;

public slots:
    void cancel();

signals:
    void loggingChanged();
    void qualityChanged();
    void maxSizeChanged();
    void bellChanged();
    void aliveChanged();
    void framesChanged();
    void errorChanged();
    void dimensionChanged();
    void viewRectChanged();
    void viewScaleChanged();
    void viewCenterChanged();
    void sshTunnelListen(const QString &addr, int port);
    void canceled();

protected:
    // Reimplemented from QQuickItem
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void adjustGeometry();
    void onAliveChanged(bool alive);
    void onErrorChanged(const QString &text);
    void onSizeChanged(int width, int height);
    void onAreaChanged(int x, int y, int width, int height);
    void emitMouseAction(const QPointF &pos, int button, bool move, bool down);
    void onStateChanged();
    void onNewConnection();

    DesktopClient *desktop_client;

    bool client_logging;
    DesktopClient::Quality client_quality;
    QSize client_maxsize;
    QString client_bell;

    bool client_alive;
    int client_frames;
    QString client_error;
    QSizeF client_dimension;

    QTransform local_remote;
    QTransform remote_local;
    QRectF paint_area;
    qreal set_scale;
    qreal view_scale;
    QPointF view_center;

    bool release_buttons;
    int mouse_buttons;
    QPoint mouse_pos;

    QString tunnel_addr;
    int tunnel_port;
    QPointer<SshSession> ssh_session;
    QWeakPointer<SshChannelPort> ssh_channel;
};

class RdpDesktopThread : public BaseThread<RdpDesktopClient>
{
    Q_OBJECT
public:
    explicit RdpDesktopThread(QObject *parent = nullptr)
        : BaseThread<RdpDesktopClient>(new RdpDesktopClient, parent) {
        connect(this, &QThread::started,
                worker(), &DesktopClient::startSession, Qt::QueuedConnection);
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                worker(), &DesktopClient::stopSession, Qt::QueuedConnection);
    }
};

class VncDesktopThread : public BaseThread<VncDesktopClient>
{
    Q_OBJECT
public:
    explicit VncDesktopThread(QObject *parent = nullptr)
        : BaseThread<VncDesktopClient>(new VncDesktopClient, parent) {
        connect(this, &QThread::started,
                worker(), &DesktopClient::startSession, Qt::QueuedConnection);
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                worker(), &DesktopClient::stopSession, Qt::QueuedConnection);
    }
};

#endif // DESKTOPVIEW_H
