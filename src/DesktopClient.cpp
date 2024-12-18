#include <QGuiApplication>
#include <QClipboard>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QFileInfo>

#include "DesktopClient.h"

//#define TRACE_DESKTOPCLIENT
#ifdef  TRACE_DESKTOPCLIENT
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

DesktopClient::DesktopClient(QObject *parent)
    : QObject(parent)
    , image_quality(QualityAve)
    , sound_effect(new QSoundEffect(this))
{
    TRACE();
    setBellSound(defaultBellSound);
    connect(sound_effect, &QSoundEffect::sourceChanged, this, &DesktopClient::bellSoundChanged);
}

DesktopClient::~DesktopClient()
{
    TRACE();
}

void DesktopClient::setLogging(bool enable)
{
    Q_UNUSED(enable)
    // do nothing by default
}

void DesktopClient::setServerUrl(const QUrl &url)
{
    TRACE_ARG(url);
    server_url = url;
}

QUrl DesktopClient::serverUrl() const
{
    return server_url;
}

QSize DesktopClient::maxSize() const
{
    return image_maxsize;
}

void DesktopClient::setMaxSize(const QSize &size)
{
    TRACE_ARG(size);
    if (!size.isEmpty() && size != image_maxsize) {
        image_maxsize = size;
        emit maxSizeChanged();
    }
}

DesktopClient::Quality DesktopClient::quality() const
{
    return image_quality;
}

void DesktopClient::setQuality(Quality quality)
{
    TRACE_ARG(quality);
    if (quality != image_quality) {
        image_quality = quality;
        emit qualityChanged();
    }
}

QString DesktopClient::bellSound() const
{
    return sound_effect->source().toLocalFile();
}

bool DesktopClient::setBellSound(const QString &wavFile)
{
    TRACE_ARG(wavFile);
    if (wavFile.isEmpty()) return false;
    QString path = wavFile;
    if (QFileInfo(wavFile).isRelative()) {
        path.prepend('/');
        path.prepend(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    }
    if (!QFileInfo::exists(path)) return false;
    sound_effect->setSource(QUrl::fromLocalFile(path));
    return sound_effect->isLoaded();
}

QImage DesktopClient::scaledImage(const QSize &size) const
{
    image_mutex.lockForRead();
    QImage img = buffer_image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    image_mutex.unlock();
    return img;
}

void DesktopClient::setBufferImage(const QImage &image)
{
    TRACE_ARG(image.size());
    image_mutex.lockForWrite();
    buffer_image = image;
    image_mutex.unlock();
}

void DesktopClient::setImageLocked(bool on)
{
    TRACE_ARG(on);
    if (on) image_mutex.lockForWrite();
    else    image_mutex.unlock();
}

bool DesktopClient::playBellSound(int duration, int frequency)
{
    Q_UNUSED(duration)
    Q_UNUSED(frequency)

    if (!sound_effect->isLoaded() || sound_effect->isPlaying()) return false;
    sound_effect->play();
    return true;
}

void DesktopClient::clipboardText(const QString &text)
{
    if (text.isEmpty()) return;
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) clipboard->setText(text);
}
