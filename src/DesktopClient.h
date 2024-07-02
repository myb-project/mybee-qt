#ifndef DESKTOPCLIENT_H
#define DESKTOPCLIENT_H

#include <QObject>
#include <QUrl>
#include <QSize>
#include <QImage>
#include <QCursor>
#include <QReadWriteLock>

class QSoundEffect;
class DesktopAction;

class DesktopClient : public QObject
{
    Q_OBJECT
public:
    static constexpr char const *defaultBellSound = "short-pulse.wav";

    explicit DesktopClient(const QUrl &serverUrl, QObject *parent = nullptr);
    virtual ~DesktopClient();

    enum Quality { QualityFast, QualityAve, QualityBest };

    virtual void setLogging(bool enable); // do nothing by default

    QUrl serverUrl() const;

    QSize maxSize() const;
    void setMaxSize(const QSize &size);

    Quality quality() const;
    void setQuality(Quality quality);

    QString bellSound() const;
    bool setBellSound(const QString &wavFile);

    QImage scaledImage(const QSize &size) const;

public slots:
    virtual void sendInputAction(const QString &text) = 0;
    virtual void sendDesktopAction(const DesktopAction &act) = 0;

signals:
    void maxSizeChanged();
    void qualityChanged();
    void bellSoundChanged();

    void aliveChanged(bool alive);
    void errorChanged(const QString &text);
    void sizeChanged(int width, int height);
    void areaChanged(int x, int y, int width, int height);
    void cursorChanged(const QCursor &cursor);

protected:
    void setBufferImage(const QImage &image);
    void setImageLocked(bool on);
    bool playBellSound(int duration, int frequency);
    void clipboardText(const QString &text);

private:
    QUrl server_url;

    QSize image_maxsize;
    Quality image_quality;

    QImage buffer_image;
    mutable QReadWriteLock image_mutex;

    QSoundEffect *sound_effect;
};

#endif // DESKTOPCLIENT_H
