#include <QSocketNotifier>
#include <QCoreApplication>
#include <QTimer>
#include <QLocale>
#include <QImage>
#include <QPixmap>
#include <QtDebug>

#include "freerdp/freerdp.h"
#include "freerdp/input.h"
#include "freerdp/gdi/gdi.h"
#include "freerdp/gdi/gfx.h"
#include "freerdp/client/cmdline.h"
#include "freerdp/client/channels.h"
#include "freerdp/client/cliprdr.h"
#ifdef Q_OS_UNIX
#include "freerdp/locale/keyboard.h"
#endif

#include "RdpDesktopClient.h"
#include "RdpAudioPlugin.h"
#include "DesktopAction.h"

//#define TRACE_RDPDESKTOPCLIENT
#ifdef TRACE_RDPDESKTOPCLIENT
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

static bool rdpLoggingEnabled = false;

static const struct localeKeymapEntry {
    int country;
    int rdp_kbd;
} localeKeymapTable[] = {
    { QLocale::Russia, KBD_RUSSIAN }, // see freerdp/locale/keyboard.h for country code
    // --- your QLocale::<Country> entry add here ---
    { 0, 0 } // End of table
};

struct MyContext {
    MyContext() : self(nullptr) {}
    rdpContext rdp_context;
    RdpDesktopClient *self;
};

static MyContext *myContext(rdpContext *context)
{
    return reinterpret_cast<MyContext*>(context);
}

static MyContext *myContext(freerdp *instance)
{
    return myContext(instance->context);
}

struct MyPointer {
    MyPointer() : index(0) {}
    rdpPointer pointer;
    int index;
};

static MyPointer *myPointer(rdpPointer *pointer)
{
    return reinterpret_cast<MyPointer*>(pointer);
}

static const MyPointer *myPointer(const rdpPointer *pointer)
{
    return reinterpret_cast<const MyPointer*>(pointer);
}

RdpDesktopClient::RdpDesktopClient(QObject *parent)
    : DesktopClient(parent)
    , client_instance(nullptr)
    , remote_width(0)
    , remote_height(0)
    , cursor_index(0)
{
    TRACE();

    connect(this, &DesktopClient::maxSizeChanged, this, [this]() {
        if (client_instance && !maxSize().isEmpty()) {
            client_instance->settings->DesktopWidth = maxSize().width();
            client_instance->settings->DesktopHeight = maxSize().height();
        }
    });
    connect(this, &DesktopClient::qualityChanged, this, [this]() {
        if (client_instance)
            setInstanceQuality(client_instance, quality());
    });
}

RdpDesktopClient::~RdpDesktopClient()
{
    TRACE();

    RdpDesktopClient::stopSession();
}

void RdpDesktopClient::setLogging(bool enable)
{
    TRACE_ARG(enable);

    rdpLoggingEnabled = enable;
    qputenv("WLOG_LEVEL", enable ? "ON" : "OFF");
}

static PVIRTUALCHANNELENTRY channelAddinLoadHook(LPCSTR pszName, LPCSTR pszSubsystem, LPCSTR pszType, DWORD dwFlags)
{
    TRACE_ARG(pszName << pszSubsystem << pszType << dwFlags);

    if (qstrcmp(pszName, "rdpsnd") == 0 && qstrcmp(pszSubsystem, "qt") == 0)
        return reinterpret_cast<PVIRTUALCHANNELENTRY>(RdpAudioPlugin::create);

    return ::freerdp_channels_load_static_addin_entry(pszName, pszSubsystem, pszType, dwFlags);
}

void RdpDesktopClient::startSession()
{
    TRACE_ARG(serverUrl());

    if (client_instance) stopSession();

    client_instance = ::freerdp_new();
    client_instance->ContextSize = sizeof(MyContext);
    client_instance->PreConnect = preConnect;
    client_instance->PostConnect = postConnect;
    client_instance->PostDisconnect = postDisconnect;

    ::freerdp_context_new(client_instance);
    myContext(client_instance)->self = this;

    if (::freerdp_register_addin_provider(::freerdp_channels_load_static_addin_entry, 0) != CHANNEL_RC_OK ||
        ::freerdp_register_addin_provider(channelAddinLoadHook, 0) != CHANNEL_RC_OK) {
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "freerdp_register_addin_provider() failed";
        emitErrorText();
        return;
    }
    auto settings = client_instance->settings;

    QString opt = serverUrl().host();
    settings->ServerHostname = qstrdup(!opt.isEmpty() ? opt.toUtf8().constData() : "localhost");
    settings->ServerPort = serverUrl().port(3389);

    opt = serverUrl().userName();
    if (opt.isEmpty()) opt = QCoreApplication::applicationName();
    settings->Username = qstrdup(opt.toUtf8().constData());

    opt = serverUrl().password();
    settings->Password = qstrdup(!opt.isEmpty() ? opt.toUtf8().constData() : "");

    if (!maxSize().isEmpty()) {
        settings->DesktopWidth = maxSize().width();
        settings->DesktopHeight = maxSize().height();
    }
    settings->EmbeddedWindow = true;
    settings->SupportGraphicsPipeline = true;
    settings->GfxAVC444 = true;
    settings->GfxAVC444v2 = true;
    settings->GfxH264 = true;
    settings->RemoteFxCodec = true;
    settings->ColorDepth = 32;
    settings->FastPathOutput = true;
    settings->FastPathInput = true;
    settings->FrameMarkerCommandEnabled = true;
    settings->SupportDynamicChannels = true;
    settings->AudioPlayback = true;
    settings->KeyboardLayout = KBD_UNITED_STATES_INTERNATIONAL;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto code = QLocale::system().country();
#else
    auto code = QLocale::system().territory();
#endif
    for (const struct localeKeymapEntry *e = localeKeymapTable; e && e->country && e->rdp_kbd; ++e) {
        if (e->country == code) {
            settings->KeyboardLayout = e->rdp_kbd;
            break;
        }
    }

    char *sound_argv[2], rdpsnd[10], sys_qt[10];
    sound_argv[0] = qstrcpy(rdpsnd, "rdpsnd");
    sound_argv[1] = qstrcpy(sys_qt, "sys:qt");
    ::freerdp_client_add_static_channel(settings, 2, sound_argv);

    setInstanceQuality(client_instance, quality());

    if (!::freerdp_connect(client_instance)) {
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "freerdp_connect() failed";
        emitErrorText();
        return;
    }
    setSocketNotifiers(true);

    emit aliveChanged(true);
}

void RdpDesktopClient::setSocketNotifiers(bool enable)
{
    if (!client_instance) return;
    if (enable) {
        int rcount = 0, wcount = 0;
        void *rfds[64], *wfds[64];
        if (!::freerdp_get_fds(client_instance, rfds, &rcount, wfds, &wcount)) {
            if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "freerdp_get_fds() failed";
            emitErrorText();
            return;
        }
        if (!::freerdp_channels_get_fds(client_instance->context->channels, client_instance, rfds, &rcount, wfds, &wcount)) {
            if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "freerdp_channels_get_fds() failed";
            emitErrorText();
            return;
        }
        if (!rcount && !wcount) {
            if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "No one socket descriptor?";
            emitErrorText();
            return;
        }
        QList<qintptr> rlist, wlist;
        const auto list = findChildren<QSocketNotifier*>(QString(), Qt::FindDirectChildrenOnly);
        for (auto sn : list) {
            if (sn->type() == QSocketNotifier::Read)       rlist.append(sn->socket());
            else if (sn->type() == QSocketNotifier::Write) wlist.append(sn->socket());
            sn->setEnabled(true);
        }
        for (int i = 0; i < rcount; i++) {
            qintptr pfd = reinterpret_cast<qintptr>(rfds[i]);
            if (!rlist.contains(pfd)) {
                TRACE_ARG("Read" << pfd);

                auto sn = new QSocketNotifier(pfd, QSocketNotifier::Read, this);
                connect(sn, &QSocketNotifier::activated, this, &RdpDesktopClient::onSocketActivated);
            }
        }
        for (int i = 0; i < wcount; i++) {
            qintptr pfd = reinterpret_cast<qintptr>(wfds[i]);
            if (!wlist.contains(pfd)) {
                TRACE_ARG("Write" << pfd);

                auto sn = new QSocketNotifier(pfd, QSocketNotifier::Write, this);
                connect(sn, &QSocketNotifier::activated, this, &RdpDesktopClient::onSocketActivated);
            }
        }
    } else {
        const auto list = findChildren<QSocketNotifier*>(QString(), Qt::FindDirectChildrenOnly);
        for (auto sn : list) {
            sn->setEnabled(false);
            sn->deleteLater();
        }
    }
}

void RdpDesktopClient::onSocketActivated()
{
    if (!client_instance) return;
    if (::freerdp_shall_disconnect(client_instance)) {
        setSocketNotifiers(false);
        ::freerdp_disconnect(client_instance);
        QTimer::singleShot(0, this, &RdpDesktopClient::freeSession);
        return;
    }
    if (!::freerdp_check_fds(client_instance)) {
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "freerdp_check_fds() failed";
        emitErrorText();
        return;
    }
    if (!::freerdp_channels_check_fds(client_instance->context->channels, client_instance)) {
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "freerdp_channels_check_fds() failed";
        emitErrorText();
        return;
    }
}

void RdpDesktopClient::stopSession()
{
    TRACE();

    if (!client_instance) return;
    setSocketNotifiers(false);
    ::freerdp_abort_connect(client_instance);
    freeSession();
}

void RdpDesktopClient::freeSession()
{
    TRACE();

    if (!client_instance) return;
    ::freerdp_context_free(client_instance);
    ::freerdp_free(client_instance);
    client_instance = nullptr;
}

void RdpDesktopClient::emitErrorText(const QString &text)
{
    TRACE_ARG(text);

    QString txt = text;
    if (txt.isEmpty()) {
        if (client_instance) {
            auto error = ::freerdp_get_last_error(client_instance->context);
            if (error) txt = ::freerdp_get_last_error_string(error);
        }
        if (txt.isEmpty()) txt = QStringLiteral("Unknown error");
    }
    stopSession();
    txt.prepend(serverUrl().toDisplayString() + ": ");
    emit errorChanged(txt);
}

void RdpDesktopClient::sendInputAction(const QString &text)
{
    TRACE_ARG(text);

    if (!client_instance || !client_instance->input) return;
    for (int i = 0; i < text.length(); i++) {
        quint32 code = text.at(i).unicode();
        ::freerdp_input_send_keyboard_event_ex(client_instance->input, true, code);
        ::freerdp_input_send_keyboard_event_ex(client_instance->input, false, code);
    }
}

static quint16 rdpWheelFlags(int delta)
{
    int value = qBound(0, qAbs(delta), 0xFF);
    quint16 flags = 0;
    flags |= PTR_FLAGS_WHEEL;
    if (delta < 0) {
        flags |= PTR_FLAGS_WHEEL_NEGATIVE;
        flags = (flags & 0xFF00) | (0x100 - value);
    } else flags |= value;
    return flags;
}

void RdpDesktopClient::sendDesktopAction(const DesktopAction &act)
{
    //TRACE_ARG(act);

    if (!client_instance || !client_instance->input) return;
    switch (act.type()) {
    case DesktopAction::ActionKey: {
#ifdef Q_OS_UNIX
        quint32 scanCode = ::freerdp_keyboard_get_rdp_scancode_from_x11_keycode(act.scanCode());
#else
        quint32 scanCode = act.scanCode();
#endif
        ::freerdp_input_send_keyboard_event_ex(client_instance->input, act.down(), scanCode);
    }   break;
    case DesktopAction::ActionMouse: {
        bool ext = false;
        quint16 flags = 0;
        switch (act.button()) {
        case Qt::LeftButton:    flags |= PTR_FLAGS_BUTTON1; break;
        case Qt::RightButton:   flags |= PTR_FLAGS_BUTTON2; break;
        case Qt::MiddleButton:  flags |= PTR_FLAGS_BUTTON3; break;
        case Qt::BackButton:    flags |= PTR_XFLAGS_BUTTON1; ext = true; break;
        case Qt::ForwardButton: flags |= PTR_XFLAGS_BUTTON2; ext = true; break;
        default:
            if (act.button() || !act.move()) {
                if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "Unexpected button" << act.button();
                return;
            }
        }
        if (act.down())      flags |= (ext ? PTR_XFLAGS_DOWN : PTR_FLAGS_DOWN);
        else if (act.move()) flags |= PTR_FLAGS_MOVE;
        if (ext) {
            ::freerdp_input_send_extended_mouse_event(client_instance->input, flags, act.posX(), act.posY());
        } else {
            ::freerdp_input_send_mouse_event(client_instance->input, flags, act.posX(), act.posY());
        }
    }   break;
    case DesktopAction::ActionWheel:
        if (act.deltaY()) {
            ::freerdp_input_send_mouse_event(client_instance->input, rdpWheelFlags(act.deltaY()), act.posX(), act.posY());
        } else if (act.deltaX()) {
            ::freerdp_input_send_mouse_event(client_instance->input, rdpWheelFlags(act.deltaX()), act.posX(), act.posY());
        }
        break;
    default:
        break;
    }
}

bool RdpDesktopClient::setInstanceQuality(freerdp *instance, Quality quality)
{
    if (!instance || !instance->settings) return false;
    quint32 quality_bits = 0;
    switch (quality) {
    case QualityBest: // DESKTOP_COMPOSITION disabled, all other enabled
        quality_bits = 0x80;
        break;
    case QualityFast: // THEMING, CURSOR_SHADOW, CURSORSETTINGS enabled, all other disabled
        quality_bits = 0x07;
        break;
    case QualityAve: // WALLPAPER, FONT_SMOOTHING, DESKTOP_COMPOSITION disabled, all other enabled
    default:
        quality_bits = 0x01;
        break;
    }
    return ::freerdp_settings_set_uint32(instance->settings, FreeRDP_PerformanceFlags, quality_bits);
}

void RdpDesktopClient::channelConnected(void *ctx, ChannelConnectedEventArgs *ea)
{
    TRACE_ARG(ea->name);

    auto context = reinterpret_cast<rdpContext*>(ctx);
    if (!context) return;

    if (qstrcmp(ea->name, RDPGFX_DVC_CHANNEL_NAME) == 0) {
        ::gdi_graphics_pipeline_init(context->gdi, (RdpgfxClientContext*)ea->pInterface);
    } else if (qstrcmp(ea->name, CLIPRDR_SVC_CHANNEL_NAME) == 0) {
        //XXX Not implemented yet
    }
    auto self = myContext(context)->self;
    if (self) self->setSocketNotifiers(true);
}

void RdpDesktopClient::channelDisconnected(void *ctx, ChannelDisconnectedEventArgs *ea)
{
    TRACE_ARG(ea->name);

    auto context = reinterpret_cast<rdpContext*>(ctx);
    if (qstrcmp(ea->name, RDPGFX_DVC_CHANNEL_NAME) == 0) {
        ::gdi_graphics_pipeline_uninit(context->gdi, (RdpgfxClientContext*)ea->pInterface);
    }
}

BOOL RdpDesktopClient::preConnect(freerdp *instance)
{
    TRACE_ARG(instance);

    auto settings = instance->settings;
#if defined(Q_OS_ANDROID)
    settings->OsMajorType = OSMAJORTYPE_ANDROID;
#elif defined(Q_OS_IOS)
    settings->OsMajorType = OSMAJORTYPE_IOS;
#elif defined(Q_OS_MAC)
    settings->OsMajorType = OSMAJORTYPE_OSX;
#elif defined(Q_OS_WIN)
    settings->OsMajorType = OSMAJORTYPE_WINDOWS;
#else
    settings->OsMajorType = OSMAJORTYPE_UNIX;
#endif
    settings->OsMinorType = OSMINORTYPE_UNSPECIFIED;
    settings->BitmapCacheEnabled = true;
    settings->OffscreenSupportLevel = 1;

    ::PubSub_SubscribeChannelConnected(instance->context->pubSub, channelConnected);
    ::PubSub_SubscribeChannelDisconnected(instance->context->pubSub, channelDisconnected);

    return ::freerdp_client_load_addins(instance->context->channels, settings);
}

BOOL RdpDesktopClient::postConnect(freerdp *instance)
{
    TRACE_ARG(instance);

    auto self = myContext(instance)->self;
    if (!self) return false;

    auto settings = instance->context->settings;
    QImage buffer = QImage(settings->DesktopWidth, settings->DesktopHeight, QImage::Format_RGBA8888);
    if (!::gdi_init_ex(instance, PIXEL_FORMAT_RGBA32, buffer.bytesPerLine(), buffer.bits(), nullptr)) {
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "gdi_init_ex() failed";
        self->emitErrorText();
        return false;
    }
    auto gdi = instance->context->gdi;
    if (!gdi || gdi->width < 0 || gdi->height < 0) {
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "gdi is invalid?";
        self->emitErrorText();
        return false;
    }
    instance->update->BeginPaint = beginPaint;
    instance->update->EndPaint = endPaint;
    instance->update->DesktopResize = desktopResize;
    instance->update->PlaySound = playSound;

    if (instance->context->graphics) {
        rdpPointer pointer;
        ::memset(&pointer, 0, sizeof(rdpPointer));
        pointer.size = sizeof(MyPointer);
        pointer.New = pointerNew;
        pointer.Free = pointerFree;
        pointer.Set = pointerSet;
        ::graphics_register_pointer(instance->context->graphics, &pointer);
    }

    ::freerdp_keyboard_init_ex(settings->KeyboardLayout, settings->KeyboardRemappingList);

    self->setBufferImage(buffer);

    if (gdi->width != self->remote_width || gdi->height != self->remote_height) {
        self->remote_width = gdi->width;
        self->remote_height = gdi->height;
        emit self->sizeChanged(self->remote_width, self->remote_height);
    }
    return true;
}

void RdpDesktopClient::postDisconnect(freerdp *instance)
{
    TRACE_ARG(instance);

    ::gdi_free(instance);
    auto self = myContext(instance)->self;
    if (self) emit self->aliveChanged(false);
}

BOOL RdpDesktopClient::beginPaint(rdpContext *context)
{
    if (!context) return false;

    auto gdi = context->gdi;
    if (!gdi || !gdi->primary || !gdi->primary->hdc || !gdi->primary->hdc->hwnd) return false;

    gdi->primary->hdc->hwnd->invalid->null = true;
    return true;
}

BOOL RdpDesktopClient::endPaint(rdpContext *context)
{
    if (!context) return false;

    auto gdi = context->gdi;
    if (!gdi || !gdi->primary || !gdi->primary->hdc || !gdi->primary->hdc->hwnd) return false;

    auto hwnd = gdi->primary->hdc->hwnd;
    if (hwnd->invalid->null || hwnd->ninvalid < 1) return true;

    auto cinvalid = hwnd->cinvalid;
    if (!cinvalid) return false;

    auto self = myContext(context)->self;
    if (!self) return false;

    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    self->setImageLocked(true);
    for (int i = 0; i < hwnd->ninvalid; i++) {
        x1 = qMin(x1, cinvalid[i].x);
        y1 = qMin(y1, cinvalid[i].y);
        x2 = qMax(x2, cinvalid[i].x + cinvalid[i].w);
        y2 = qMax(y2, cinvalid[i].y + cinvalid[i].h);
    }
    hwnd->invalid->null = true;
    hwnd->ninvalid = 0;
    self->setImageLocked(false);

    emit self->areaChanged(x1, y1, x2 - x1, y2 - y1);
    return true;
}

BOOL RdpDesktopClient::desktopResize(rdpContext *context)
{
    TRACE_ARG(context);

    auto self = myContext(context)->self;
    if (!self) return false;

    auto settings = context->settings;
    auto gdi = context->gdi;

    QImage buffer = QImage(settings->DesktopWidth, settings->DesktopHeight, QImage::Format_RGBA8888);
    if (!::gdi_resize_ex(gdi, settings->DesktopWidth, settings->DesktopHeight,
                         buffer.bytesPerLine(), PIXEL_FORMAT_RGBA32, buffer.bits(), nullptr)) {
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "gdi_resize_ex() failed";
        self->emitErrorText();
        return false;
    }

    self->setBufferImage(buffer);

    if ((int)settings->DesktopWidth != self->remote_width || (int)settings->DesktopHeight != self->remote_height) {
        self->remote_width = settings->DesktopWidth;
        self->remote_height = settings->DesktopHeight;
        emit self->sizeChanged(self->remote_width, self->remote_height);
    }
    return true;
}

BOOL RdpDesktopClient::playSound(rdpContext *context, const PLAY_SOUND_UPDATE *play)
{
    TRACE();

    auto self = myContext(context)->self;
    return (self && play) ? self->playBellSound(play->duration, play->frequency) : false;
}

BOOL RdpDesktopClient::pointerNew(rdpContext *context, rdpPointer *pointer)
{
    TRACE();

    if (!context || !pointer || !context->gdi) return false;
    auto self = myContext(context)->self;
    if (!self) return false;

    auto data = new BYTE[pointer->width * pointer->height * 4];
    if (!::freerdp_image_copy_from_pointer_data(data, PIXEL_FORMAT_ARGB32, 0, 0, 0,
                                                pointer->width, pointer->height,
                                                pointer->xorMaskData, pointer->lengthXorMask,
                                                pointer->andMaskData, pointer->lengthAndMask,
                                                pointer->xorBpp, &context->gdi->palette)) {
        delete[] data;
        if (rdpLoggingEnabled) qWarning() << Q_FUNC_INFO << "freerdp_image_copy_from_pointer_data() failed";
        return false;
    }
    QPixmap pixmap(QPixmap::fromImage(QImage(data, pointer->width, pointer->height, QImage::Format_ARGB32)));
    delete[] data;

    self->cursor_map.insert(self->cursor_index, QCursor(pixmap, pointer->xPos, pointer->yPos));
    myPointer(pointer)->index = self->cursor_index;
    self->cursor_index++;
    return true;
}

void RdpDesktopClient::pointerFree(rdpContext *context, rdpPointer *pointer)
{
    TRACE();

    auto self = myContext(context)->self;
    auto ptr = myPointer(pointer);
    if (self && ptr) self->cursor_map.remove(ptr->index);
}

BOOL RdpDesktopClient::pointerSet(rdpContext *context, const rdpPointer *pointer)
{
    TRACE();

    auto self = myContext(context)->self;
    auto ptr = myPointer(pointer);
    if (self && ptr) {
        int index = ptr->index;
        if (self->cursor_map.contains(index)) {
            emit self->cursorChanged(self->cursor_map[index]);
            return true;
        }
    }
    return false;
}
