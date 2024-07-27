#include <QPainter>
#include <QTimer>
#include <QMetaObject>
#include <QtMath>

#include "DesktopView.h"
#include "DesktopAction.h"

//#define TRACE_DESKTOPVIEW
#ifdef TRACE_DESKTOPVIEW
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

DesktopView::DesktopView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , desktop_client(nullptr)
    , client_logging(false)
    , client_quality(DesktopClient::QualityAve)
    , client_bell(DesktopClient::defaultBellSound)
    , client_alive(false)
    , client_frames(0)
    , set_scale(0.0)
    , view_scale(0.0)
    , release_buttons(false)
    , mouse_buttons(0)
{
    TRACE();

    qRegisterMetaType<DesktopAction>("DesktopAction");

    setOpaquePainting(true);
    setAntialiasing(true);
    setRenderTarget(QQuickPaintedItem::Image);

    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setFlag(QQuickItem::ItemAcceptsInputMethod, true);

    auto timer = new QTimer(this);
    timer->setInterval(200);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &DesktopView::adjustGeometry);
    connect(this, &QQuickItem::widthChanged, timer, QOverload<>::of(&QTimer::start));
    connect(this, &QQuickItem::heightChanged, timer, QOverload<>::of(&QTimer::start));
}

DesktopView::~DesktopView()
{
    TRACE();
}

bool DesktopView::logging() const
{
    return client_logging;
}

void DesktopView::setLogging(bool enable)
{
    TRACE_ARG(enable);

    if (enable != client_logging) {
        client_logging = enable;
        if (desktop_client)
            QMetaObject::invokeMethod(desktop_client, "setLogging", Q_ARG(bool, client_logging));
        emit loggingChanged();
    }
}

int DesktopView::quality() const
{
    return client_quality;
}

void DesktopView::setQuality(int quality)
{
    TRACE_ARG(quality);

    int q = qBound((int)DesktopClient::QualityFast, quality, (int)DesktopClient::QualityBest);
    if (q != client_quality) {
        client_quality = (DesktopClient::Quality)q;
        if (desktop_client)
            QMetaObject::invokeMethod(desktop_client, "setQuality", Q_ARG(int, client_quality));
        emit qualityChanged();
    }
}

QSize DesktopView::maxSize() const
{
    return client_maxsize;
}

void DesktopView::setMaxSize(const QSize &size)
{
    TRACE_ARG(size);

    if (!size.isEmpty() && size != client_maxsize) {
        client_maxsize = size;
        if (desktop_client)
            QMetaObject::invokeMethod(desktop_client, "setMaxSize", Q_ARG(QSize, client_maxsize));
        emit maxSizeChanged();
    }
}

QString DesktopView::bell() const
{
    return client_bell;
}

void DesktopView::setBell(const QString &wav_file)
{
    TRACE_ARG(wav_file);

    if (!wav_file.isEmpty() && wav_file != client_bell) {
        client_bell = wav_file;
        if (desktop_client)
            QMetaObject::invokeMethod(desktop_client, "setBellSound", Q_ARG(QString, client_bell));
        emit bellChanged();
    }
}

bool DesktopView::start(const QUrl &url)
{
    TRACE_ARG(url);

    if (!url.isValid() || url.host().isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Invalid URL specified";
        return false;
    }

    QThread *thread = nullptr;
    DesktopClient *client = nullptr;
    QString scheme = url.scheme().toLower();
    if (scheme == "rdp") {
        auto rdp = new RdpDesktopThread(url, this);
        thread = rdp;
        client = rdp->worker();
        release_buttons = true;
    } else if (scheme == "vnc") {
        auto vnc = new VncDesktopThread(url, this);
        thread = vnc;
        client = vnc->worker();
        release_buttons = false;
    }
    if (!thread || !client) {
        qWarning() << Q_FUNC_INFO << "Unsupported URL scheme" << scheme;
        return false;
    }

    client->setLogging(client_logging);
    client->setQuality(client_quality);
    if (!client_maxsize.isEmpty()) client->setMaxSize(client_maxsize);
    if (!client_bell.isEmpty()) client->setBellSound(client_bell);

    connect(client, &DesktopClient::aliveChanged, this, &DesktopView::onAliveChanged, Qt::QueuedConnection);
    connect(client, &DesktopClient::errorChanged, this, &DesktopView::onErrorChanged, Qt::QueuedConnection);
    connect(client, &DesktopClient::sizeChanged,  this, &DesktopView::onSizeChanged, Qt::QueuedConnection);
    connect(client, &DesktopClient::areaChanged,  this, &DesktopView::onAreaChanged, Qt::QueuedConnection);
    connect(client, &DesktopClient::cursorChanged, this, &QQuickItem::setCursor, Qt::QueuedConnection);

    desktop_client = client;
    thread->start();
    return true;
}

bool DesktopView::alive() const
{
    return client_alive;
}

int DesktopView::frames() const
{
    return client_frames;
}

QString DesktopView::error() const
{
    return client_error;
}

QSizeF DesktopView::dimension() const
{
    return client_dimension;
}

QRectF DesktopView::viewRect() const
{
    return paint_area;
}

qreal DesktopView::viewScale() const
{
    return view_scale;
}

void DesktopView::setViewScale(qreal scale)
{
    TRACE_ARG(scale);

    qreal s = qBound(0.2, scale, 2.0);
    if (s != set_scale) {
        set_scale = s;
        adjustGeometry();
    }
}

QPointF DesktopView::viewCenter() const
{
    return view_center;
}

void DesktopView::setViewCenter(const QPointF &center)
{
    TRACE_ARG(center);

    if (center != view_center) {
        view_center = center;
        adjustGeometry();
    }
}

void DesktopView::adjustGeometry()
{
    const QSizeF local_size(width(), height());
    TRACE_ARG(local_size << client_dimension);

    if (local_size.isEmpty() || client_dimension.isEmpty()) return;

    const QRectF local_rect(0, 0, width(), height());
    qreal prev_view_scale = view_scale;
    QPointF prev_view_center = view_center;
    QRectF prev_paint_area = paint_area;

    const QSizeF scaled_size = client_dimension.scaled(local_size, Qt::KeepAspectRatio);
    qreal sx = qMin(scaled_size.width() / client_dimension.width(), 1.0);
    qreal sy = qMin(scaled_size.height() / client_dimension.height(), 1.0);
    if (sx < set_scale) sx = set_scale;
    if (sy < set_scale) sy = set_scale;
    view_scale = (sx + sy) / 2.0;

    local_remote.reset();
    local_remote.translate(client_dimension.width() / 2 + view_center.x(),
                           client_dimension.height() / 2 + view_center.y());
    local_remote.scale(1.0 / sx, 1.0 / sy);
    local_remote.translate(-local_size.width() / 2, -local_size.height() / 2);
    const QRectF view_rect = local_remote.mapRect(local_rect);

    sx = 0.0;
    if (client_dimension.width() >= view_rect.width()) {
        qreal left = -view_rect.x();
        qreal right = view_rect.right() - client_dimension.width();
        if (left > 0) sx = left;
        else if (right > 0) sx = -right;
    } else sx = -view_rect.x() - (view_rect.width() - client_dimension.width()) / 2;

    sy = 0.0;
    if (client_dimension.height() >= view_rect.height()) {
        qreal top = -view_rect.y();
        qreal bottom = view_rect.bottom() - client_dimension.height();
        if (top > 0) sy = top;
        else if (bottom > 0) sy = -bottom;
    } else sy = -view_rect.y() - (view_rect.height() - client_dimension.height()) / 2;

    if (sx >= 1.0 || sy >= 1.0) {
        view_center += QPointF(sx, sy);
        local_remote *= QTransform::fromTranslate(sx, sy);
    }

    remote_local = local_remote.inverted();
    QRectF rect = remote_local.mapRect(QRectF(0, 0, client_dimension.width(), client_dimension.height()));
    paint_area = rect.intersected(local_rect);

    if (paint_area != prev_paint_area) emit viewRectChanged();
    if (view_scale != prev_view_scale) emit viewScaleChanged();
    if (view_center != prev_view_center) emit viewCenterChanged();

    QQuickItem::update();
}

void DesktopView::paint(QPainter *painter)
{
    if (!paint_area.isValid()) return;

    QRect rect = paint_area.toRect();
    QImage image = desktop_client ? desktop_client->scaledImage(rect.size()) : QImage();
    painter->drawImage(rect, image);
}

void DesktopView::onAliveChanged(bool alive)
{
    TRACE_ARG(alive);

    if (alive != client_alive) {
        client_alive = alive;
        if (client_frames) {
            client_frames = 0;
            emit framesChanged();
        }
        emit aliveChanged();
    }
}

void DesktopView::onErrorChanged(const QString &text)
{
    TRACE_ARG(text);

    if (text != client_error) {
        client_error = text;
        emit errorChanged();
    }
}

void DesktopView::onSizeChanged(int width, int height)
{
    TRACE_ARG(width << height);

    if (width < 1 || height < 1) return;
    if (width != qRound(client_dimension.width()) || height != qRound(client_dimension.height())) {
        client_dimension.setWidth(width);
        client_dimension.setHeight(height);
        adjustGeometry();
        emit dimensionChanged();
    }
}

void DesktopView::onAreaChanged(int x, int y, int width, int height)
{
    TRACE_ARG(x << y << width << height);

    if (x < 0 || y < 0 || width < 1 || height < 1) return;
    if (!client_dimension.isEmpty()) QQuickItem::update();
    else onSizeChanged(width, height);

    if (width >= qCeil(client_dimension.width()) || height >= qCeil(client_dimension.height())) {
        if (!++client_frames) client_frames++; // skip over null
        emit framesChanged();
    }
}

QVariant DesktopView::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (query != Qt::ImHints) return QQuickPaintedItem::inputMethodQuery(query);
    return int(Qt::ImhHiddenText | Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
}

void DesktopView::inputMethodEvent(QInputMethodEvent *event)
{
    if (!client_frames || !hasActiveFocus()) {
        event->ignore();
        return;
    }
    QString text = event->commitString();
    if (!text.isEmpty() && desktop_client) {
        QMetaObject::invokeMethod(desktop_client, "sendInputAction",
                                  Q_ARG(QString, text));
    }
    event->accept();
}

void DesktopView::keyPressEvent(QKeyEvent *event)
{
    if (!client_frames || !hasActiveFocus()) {
        event->ignore();
        return;
    }
    DesktopAction act(event->key(), event->nativeScanCode(), event->nativeVirtualKey(), true);
    if (act.isValid() && desktop_client)
        QMetaObject::invokeMethod(desktop_client, "sendDesktopAction", Q_ARG(DesktopAction, act));
    event->accept();
}

void DesktopView::keyReleaseEvent(QKeyEvent *event)
{
    if (!client_frames || !hasActiveFocus()) {
        event->ignore();
        return;
    }
    DesktopAction act(event->key(), event->nativeScanCode(), event->nativeVirtualKey(), false);
    if (act.isValid() && desktop_client)
        QMetaObject::invokeMethod(desktop_client, "sendDesktopAction", Q_ARG(DesktopAction, act));
    event->accept();
}

void DesktopView::emitMouseAction(const QPointF &point, int button, bool move, bool down)
{
    if (client_dimension.isEmpty() || !paint_area.contains(point)) return;
    QPoint pos = local_remote.map(point).toPoint();
    if (!pos.isNull() && (!move || (pos - mouse_pos).manhattanLength() > mouseMoveJitter) && desktop_client) {
        DesktopAction act(pos.x(), pos.y(), button, move, down);
        if (act.isValid())
            QMetaObject::invokeMethod(desktop_client, "sendDesktopAction", Q_ARG(DesktopAction, act));
    }
    mouse_pos = pos;
}

void DesktopView::hoverMoveEvent(QHoverEvent *event)
{
    if (client_frames && hasActiveFocus()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        emitMouseAction(event->pos(), 0, true, false);
#else
        emitMouseAction(event->position(), 0, true, false);
#endif
        event->accept();
    } else event->ignore();
}

void DesktopView::mousePressEvent(QMouseEvent *event)
{
    if (!hasFocus()) setFocus(true);
    if (!client_frames || !event->buttons()) {
        event->ignore();
        return;
    }
    if (!hasActiveFocus()) forceActiveFocus();

    mouse_buttons = event->buttons();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    emitMouseAction(event->localPos(), mouse_buttons, false, true);
#else
    emitMouseAction(event->position(), mouse_buttons, false, true);
#endif
    event->accept();
}

void DesktopView::mouseMoveEvent(QMouseEvent *event)
{
    if (client_frames && hasActiveFocus() && mouse_buttons) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        emitMouseAction(event->localPos(), mouse_buttons, true, true);
#else
        emitMouseAction(event->position(), mouse_buttons, true, true);
#endif
        event->accept();
    } else event->ignore();
}

void DesktopView::mouseReleaseEvent(QMouseEvent *event)
{
    if (client_frames && hasActiveFocus() && (release_buttons || mouse_buttons)) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        emitMouseAction(event->localPos(), mouse_buttons, false, false);
#else
        emitMouseAction(event->position(), mouse_buttons, false, false);
#endif
        mouse_buttons = 0;
        event->accept();
    } else event->ignore();
}

void DesktopView::wheelEvent(QWheelEvent *event)
{
    if (client_frames && hasActiveFocus()) {
        QPoint pos = local_remote.map(event->position()).toPoint();
        //QPoint delta = local_remote.map(event->angleDelta());
        QPoint delta = event->angleDelta();
        if (!pos.isNull() && !delta.isNull() && desktop_client) {
            DesktopAction act(pos.x(), pos.y(), delta.x(), delta.y());
            if (act.isValid())
                QMetaObject::invokeMethod(desktop_client, "sendDesktopAction", Q_ARG(DesktopAction, act));
        }
        event->accept();
    } else event->ignore();
}
