#ifndef VNCDESKTOPCLIENT_H
#define VNCDESKTOPCLIENT_H

#include <QRect>
#include <QSet>

#include "rfb/rfbclient.h"
#include "DesktopClient.h"

class VncDesktopClient : public DesktopClient
{
    Q_OBJECT
public:
    explicit VncDesktopClient(const QUrl &url, QObject *parent = nullptr);
    ~VncDesktopClient();

    void setLogging(bool enable) override;
    void startSession();
    void stopSession();

public slots:
    void sendInputAction(const QString &text) override;
    void sendDesktopAction(const DesktopAction &act) override;

private:
    static bool setInstanceQuality(rfbClient *instance, Quality quality);
    static void *dataTag();
    static rfbBool mallocFrameBuffer(rfbClient *instance);
    static void gotFrameBufferUpdate(rfbClient *instance, int x, int y, int w, int h);
    static void finishedFrameBufferUpdate(rfbClient *instance);
    static char *getPassword(rfbClient *instance);
    static rfbCredential *getCredential(rfbClient *instance, int type);
    static void gotXCutText(rfbClient *instance, const char *txt, int len);
    static void gotCursorShape(rfbClient *instance, int x, int y, int w, int h, int b);
    static void bell(rfbClient *instance);

    rfbClient *client_instance;

    bool remote_setsize;
    int remote_width, remote_height;
    QRect update_area;

    QSet<quint32> key_modifiers;
    int button_mask;
    int wheel_vr;
    int wheel_hr;
};

#endif // VNCDESKTOPCLIENT_H
