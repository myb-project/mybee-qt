#ifndef RDPDESKTOPCLIENT_H
#define RDPDESKTOPCLIENT_H

#include <QMap>

#include <freerdp/freerdp.h>

#include "DesktopClient.h"

class RdpDesktopClient : public DesktopClient
{
    Q_OBJECT
public:
    explicit RdpDesktopClient(const QUrl &url, QObject *parent = nullptr);
    ~RdpDesktopClient();

    void setLogging(bool enable) override;
    void startSession();
    void stopSession();

public slots:
    void sendInputAction(const QString &text) override;
    void sendDesktopAction(const DesktopAction &act) override;

private:
    static bool setInstanceQuality(freerdp *instance, Quality quality);
    static void channelConnected(void *ctx, ChannelConnectedEventArgs *ea);
    static void channelDisconnected(void *ctx, ChannelDisconnectedEventArgs *ea);
    static BOOL preConnect(freerdp *instance);
    static BOOL postConnect(freerdp *instance);
    static void postDisconnect(freerdp *instance);
    static BOOL beginPaint(rdpContext *context);
    static BOOL endPaint(rdpContext *context);
    static BOOL desktopResize(rdpContext *context);
    static BOOL playSound(rdpContext *context, const PLAY_SOUND_UPDATE *play);

    static BOOL pointerNew(rdpContext *context, rdpPointer *pointer);
    static void pointerFree(rdpContext *context, rdpPointer *pointer);
    static BOOL pointerSet(rdpContext *context, const rdpPointer *pointer);
    //static BOOL pointerSetNull(rdpContext *context);
    //static BOOL pointerSetDefault(rdpContext *context);
    //static BOOL pointerSetPosition(rdpContext *context, UINT32 x, UINT32 y);

    void setSocketNotifiers(bool enable);
    void onSocketActivated();
    void freeSession();
    void emitErrorText(const QString &text = QString());

    freerdp *client_instance;

    int remote_width, remote_height;

    QMap<int, QCursor> cursor_map;
    int cursor_index;
};

#endif // RDPDESKTOPCLIENT_H
