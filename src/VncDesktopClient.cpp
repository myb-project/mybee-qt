#include <QSocketNotifier>
#include <QCoreApplication>
#include <QPixmap>
#include <QBitmap>
#include <QRgb>
#include <QtDebug>
#include <QIODevice>

#include "VncDesktopClient.h"
#include "DesktopAction.h"

//#define TRACE_VNCDESKTOPCLIENT
#ifdef TRACE_VNCDESKTOPCLIENT
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

static bool vncLoggingEnabled = false;

static const struct controlKeysEntry {
    int qt_key;
    quint32 vnc_key;
} controlKeysTable[] = {
    { Qt::Key_Escape,       XK_Escape       },
    { Qt::Key_Tab,          XK_Tab          },
    { Qt::Key_Backspace,    XK_BackSpace    },
    { Qt::Key_Return,       XK_Return       },
    { Qt::Key_Enter,        XK_Return       },
    { Qt::Key_Insert,       XK_Insert       },
    { Qt::Key_Delete,       XK_Delete       },
    { Qt::Key_Pause,        XK_Pause        },
    { Qt::Key_Print,        XK_Print        },
    { Qt::Key_SysReq,       XK_Sys_Req      },
    { Qt::Key_Clear,        XK_Clear        },
    { Qt::Key_Home,         XK_Home         },
    { Qt::Key_End,          XK_End          },
    { Qt::Key_Left,         XK_Left         },
    { Qt::Key_Up,           XK_Up           },
    { Qt::Key_Right,        XK_Right        },
    { Qt::Key_Down,         XK_Down         },
    { Qt::Key_PageUp,       XK_Page_Up      },
    { Qt::Key_PageDown,     XK_Page_Down    },
    { Qt::Key_Shift,        XK_Shift_L      },
    { Qt::Key_Control,      XK_Control_L    },
    { Qt::Key_Meta,         XK_Meta_L       },
    { Qt::Key_Alt,          XK_Alt_L        },
    { Qt::Key_AltGr,        XK_Alt_R        },
    { Qt::Key_CapsLock,     XK_Caps_Lock    },
    { Qt::Key_NumLock,      XK_Num_Lock     },
    { Qt::Key_ScrollLock,   XK_Scroll_Lock  },
    { Qt::Key_Super_L,      XK_Super_L      },
    { Qt::Key_Super_R,      XK_Super_R      },
    { Qt::Key_Menu,         XK_Menu         },
    { Qt::Key_Hyper_L,      XK_Hyper_L      },
    { Qt::Key_Hyper_R,      XK_Hyper_R      },
    { Qt::Key_Help,         XK_Help         },
    { Qt::Key_Exclam,       XK_exclam       },
    { Qt::Key_QuoteDbl,     XK_quotedbl     },
    { Qt::Key_NumberSign,   XK_numbersign   },
    { Qt::Key_Dollar,       XK_dollar       },
    { Qt::Key_Percent,      XK_percent      },
    { Qt::Key_Ampersand,    XK_ampersand    },
    { Qt::Key_Apostrophe,   XK_apostrophe   },
    { Qt::Key_ParenLeft,    XK_parenleft    },
    { Qt::Key_ParenRight,   XK_parenright   },
    { Qt::Key_Asterisk,     XK_asterisk     },
    { Qt::Key_Plus,         XK_plus         },
    { Qt::Key_Comma,        XK_comma        },
    { Qt::Key_Minus,        XK_minus        },
    { Qt::Key_Period,       XK_period       },
    { Qt::Key_Slash,        XK_slash        },
    { 0, 0 } // End of table
};

static quint32 qtKeyToVnc(int key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z)    return XK_a + key - Qt::Key_A;
    if (key >= Qt::Key_0 && key <= Qt::Key_9)    return XK_0 + key - Qt::Key_0;
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35) return XK_F1 + key - Qt::Key_F1;
    for (const struct controlKeysEntry *e = controlKeysTable; e && e->qt_key && e->vnc_key; ++e) {
        if (e->qt_key == key) return e->vnc_key;
    }
    return 0;
}

static void reportVncLog(const char *format, ...)
{
    if (!vncLoggingEnabled) return;

    va_list args;
    va_start(args, format);
    QString text = QString::vasprintf(format, args);
    va_end(args);
    qInfo() << "VNC-Log:" << text.trimmed();
}

static void reportVncErr(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    QString text = QString::vasprintf(format, args);
    va_end(args);
    qWarning() << "VNC-Error:" << text.trimmed();
}

VncDesktopClient::VncDesktopClient(QObject *parent)
    : DesktopClient(parent)
    , client_instance(nullptr)
    , remote_setsize(false)
    , remote_width(0)
    , remote_height(0)
    , button_mask(0)
    , wheel_vr(0)
    , wheel_hr(0)
{
    TRACE();

    ::rfbClientLog = reportVncLog;
    ::rfbClientErr = reportVncErr;

    connect(this, &DesktopClient::maxSizeChanged, this, [this]() {
        if (client_instance && remote_setsize && !maxSize().isEmpty())
            ::SendExtDesktopSize(client_instance, maxSize().width(), maxSize().height());
    });
    connect(this, &DesktopClient::qualityChanged, this, [this]() {
        if (client_instance)
            setInstanceQuality(client_instance, quality());
    });
}

VncDesktopClient::~VncDesktopClient()
{
    TRACE();

    if (client_instance) VncDesktopClient::stopSession();
}

void VncDesktopClient::setLogging(bool enable)
{
    TRACE_ARG(enable);

    vncLoggingEnabled = enable;
}

void VncDesktopClient::startSession()
{
    TRACE();
    if (client_instance) stopSession();
    update_area = QRect();

    client_instance = ::rfbGetClient(8, 3, 4);
    client_instance->canHandleNewFBSize = true;
    client_instance->MallocFrameBuffer = mallocFrameBuffer;
    client_instance->GotFrameBufferUpdate = gotFrameBufferUpdate;
    client_instance->FinishedFrameBufferUpdate = finishedFrameBufferUpdate;
    client_instance->GetPassword = getPassword;
    client_instance->GetCredential = getCredential;
    client_instance->GotXCutText = gotXCutText;
    client_instance->GotCursorShape = gotCursorShape;
    client_instance->Bell = bell;

    ::rfbClientSetClientData(client_instance, dataTag(), this);

    QString opt = serverUrl().host();
    client_instance->serverHost = qstrdup(!opt.isEmpty() ? opt.toUtf8().constData() : "localhost");
    client_instance->serverPort = serverUrl().port(5900);

    TRACE_ARG("rfbInitClient()" << client_instance->serverHost << client_instance->serverPort);
    if (!::rfbInitClient(client_instance, nullptr, nullptr)) {
        client_instance = nullptr;  // rfbInitClient has already freed the client struct
        emit errorChanged(serverUrl().host() + ": Connection failed");
        return;
    }
    TRACE_ARG("rfbInitClient() Done");

    if (client_instance->sock != RFB_INVALID_SOCKET) {
        auto sn = new QSocketNotifier(client_instance->sock, QSocketNotifier::Read, this);
        connect(sn, &QSocketNotifier::activated, this, [this]() {
            if (client_instance && !::HandleRFBServerMessage(client_instance)) {
                emit errorChanged(serverUrl().host() + ": Connection aborted");
            }
        });
    }

    remote_setsize = ::SupportsClient2Server(client_instance, rfbSetDesktopSize);
    if (!remote_setsize) {
        if (vncLoggingEnabled)
            qWarning() << Q_FUNC_INFO << "The server does not support resizing the desktop";
    } else if (!maxSize().isEmpty()) {
        ::SendExtDesktopSize(client_instance, maxSize().width(), maxSize().height());
    }
    emit aliveChanged(true);
}

void VncDesktopClient::stopSession()
{
    TRACE_ARG(client_instance);

    if (!client_instance) return;

    const auto list = findChildren<QSocketNotifier*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto sn : list) {
        sn->setEnabled(false);
        sn->deleteLater();
    }
    ::rfbClientCleanup(client_instance);
    client_instance = nullptr;
    emit aliveChanged(false);
}

void VncDesktopClient::sendInputAction(const QString &text)
{
    TRACE_ARG(text);

    if (!client_instance) return;
    for (int i = 0; i < text.length(); i++) {
        quint32 code = text.at(i).unicode();
        ::SendKeyEvent(client_instance, code, true);
        ::SendKeyEvent(client_instance, code, false);
    }
}

static bool isControlKey(quint32 virtualKey)
{
    return (virtualKey == XK_Shift_L   || virtualKey == XK_Shift_R   ||
            virtualKey == XK_Control_L || virtualKey == XK_Control_R ||
            virtualKey == XK_Alt_L     || virtualKey == XK_Alt_R     ||
            virtualKey == XK_Meta_L    || virtualKey == XK_Meta_R    ||
            virtualKey == XK_Super_L   || virtualKey == XK_Super_R   ||
            virtualKey == XK_Hyper_L   || virtualKey == XK_Hyper_R);
}

void VncDesktopClient::sendDesktopAction(const DesktopAction &act)
{
    TRACE_ARG(act);

    if (!client_instance) return;
    switch (act.type()) {
    case DesktopAction::ActionKey: {
        quint32 code = 0;
        if (act.key() == Qt::Key_Backtab) {
            code = XK_Tab; // since shift modifier already handled before
        } else if (isControlKey(act.virtualKey())) {
            if (act.down()) {
                key_modifiers.insert(act.virtualKey());
            } else if (key_modifiers.contains(act.virtualKey())) {
                key_modifiers.remove(act.virtualKey());
            } else if (!key_modifiers.isEmpty()) {
                for (auto it = key_modifiers.constBegin(); it != key_modifiers.constEnd(); ++it) {
                    ::SendKeyEvent(client_instance, *it, false);
                }
                key_modifiers.clear();
            }
            code = act.virtualKey();
        } else {
            code = qtKeyToVnc(act.key());
            if (!code) code = QChar(act.virtualKey()).unicode();
        }
        if (code) ::SendKeyEvent(client_instance, code, act.down());
        else if (vncLoggingEnabled) qWarning() << Q_FUNC_INFO << "Unexpected key" << act.key();
    }   break;
    case DesktopAction::ActionMouse:
        if (!act.move()) {
            int button = act.button();
            if (act.down()) {
                if (button & Qt::LeftButton)   button_mask |= 0x01;
                if (button & Qt::MiddleButton) button_mask |= 0x02;
                if (button & Qt::RightButton)  button_mask |= 0x04;
                if (button & Qt::ExtraButton1) button_mask |= 0x80;
            } else {
                if (button & Qt::LeftButton)   button_mask &= 0xfe;
                if (button & Qt::MiddleButton) button_mask &= 0xfd;
                if (button & Qt::RightButton)  button_mask &= 0xfb;
                if (button & Qt::ExtraButton1) button_mask &= ~0x80;
            }
        }
        ::SendPointerEvent(client_instance, act.posX(), act.posY(), button_mask);
        break;
    case DesktopAction::ActionWheel: {
        int pos_x = act.posX(), pos_y = act.posY();
        int acc_v = (act.deltaY() < 0) == (wheel_vr < 0) ? wheel_vr : 0;
        int acc_h = (act.deltaX() < 0) == (wheel_hr < 0) ? wheel_hr : 0;
        int ver_ticks = (act.deltaY() + acc_v) / 120;
        int hor_ticks = (act.deltaX() + acc_h) / 120;
        int ext = ver_ticks < 0 ? 0x10 : 0x08;
        for (int i = 0; i < qAbs(ver_ticks); i++) {
            ::SendPointerEvent(client_instance, pos_x, pos_y, ext | button_mask);
            ::SendPointerEvent(client_instance, pos_x, pos_y, button_mask);
        }
        ext = hor_ticks < 0 ? 0x40 : 0x20;
        for (int i = 0; i < qAbs(hor_ticks); i++) {
            ::SendPointerEvent(client_instance, pos_x, pos_y, ext | button_mask);
            ::SendPointerEvent(client_instance, pos_x, pos_y, button_mask);
        }
        wheel_vr = (act.deltaY() + acc_v) % 120;
        wheel_hr = (act.deltaX() + acc_h) % 120;
    }   break;
    default:
        break;
    }
}

bool VncDesktopClient::setInstanceQuality(rfbClient *instance, Quality quality)
{
    if (!instance) return false;
    instance->appData.enableJPEG = true;
    instance->appData.useBGR233 = 0;
    switch (quality) {
    case QualityBest:
        instance->appData.encodingsString = "copyrect zlib hextile raw";
        instance->appData.compressLevel = 1;
        instance->appData.qualityLevel = 9;
        break;
    case QualityFast:
        instance->appData.useBGR233 = 1;
        instance->appData.encodingsString = "copyrect zrle ultra zlib hextile corre rre raw";
        instance->appData.compressLevel = 9;
        instance->appData.qualityLevel = 1;
        break;
    case QualityAve:
    default:
        instance->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
        instance->appData.compressLevel = 3;
        instance->appData.qualityLevel = 6;
        break;
    }
    return ::SetFormatAndEncodings(instance);
}

void *VncDesktopClient::dataTag()
{
    return reinterpret_cast<void*>(dataTag);
}

rfbBool VncDesktopClient::mallocFrameBuffer(rfbClient *instance)
{
    TRACE_ARG(instance->width << instance->height);

    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (!self) return false;

    bool size_changed = ((instance->width > 0 && instance->width != self->remote_width) ||
                         (instance->height > 0 && instance->height != self->remote_height));
    self->remote_width = instance->width;
    self->remote_height = instance->height;

    instance->updateRect.x = 0;
    instance->updateRect.y = 0;
    instance->updateRect.w = self->remote_width;
    instance->updateRect.h = self->remote_height;
    QImage buffer = QImage(self->remote_width, self->remote_height, QImage::Format_RGB32);
    instance->frameBuffer = buffer.bits();
    instance->width = buffer.bytesPerLine() / 4;
    instance->format.bitsPerPixel = buffer.depth();
    instance->format.redShift = 16;
    instance->format.greenShift = 8;
    instance->format.blueShift = 0;
    instance->format.redMax = 0xff;
    instance->format.greenMax = 0xff;
    instance->format.blueMax = 0xff;
    instance->appData.useRemoteCursor = true;

    if (!setInstanceQuality(instance, self->quality())) {
        if (vncLoggingEnabled) qWarning() << Q_FUNC_INFO << "SetFormatAndEncodings() failed";
        return false;
    }

    self->setBufferImage(buffer);

    if (size_changed)
        emit self->sizeChanged(self->remote_width, self->remote_height);
    return true;
}

void VncDesktopClient::gotFrameBufferUpdate(rfbClient *instance, int x, int y, int w, int h)
{
    //TRACE_ARG(x << y << w << h);

    if (w < 1 && h < 1) return;
    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (self) self->update_area |= QRect(x, y, w, h);
}

void VncDesktopClient::finishedFrameBufferUpdate(rfbClient *instance)
{
    //TRACE();

    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (!self) return;

    int x = self->update_area.x();
    int y = self->update_area.y();
    int width = self->update_area.width();
    int height = self->update_area.height();
    self->update_area = QRect();
    if (x >= 0 && y >= 0 && width > 0 && height > 0)
        emit self->areaChanged(x, y, width, height);
}

char *VncDesktopClient::getPassword(rfbClient *instance)
{
    TRACE();

    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (!self || self->serverUrl().password().isEmpty()) return qstrdup("");
    return qstrdup(self->serverUrl().password().toUtf8().constData());
}

rfbCredential *VncDesktopClient::getCredential(rfbClient *instance, int type)
{
    TRACE_ARG(type);

    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (!self || type != rfbCredentialTypeUser) {
        if (vncLoggingEnabled) qWarning() << Q_FUNC_INFO << "Unexpected credential type" << type;
        return nullptr;
    }
    auto cred = new rfbCredential;

    QString opt = self->serverUrl().userName();
    if (opt.isEmpty()) opt = QCoreApplication::applicationName();
    cred->userCredential.username = qstrdup(opt.toUtf8().constData());

    opt = self->serverUrl().password();
    cred->userCredential.password = qstrdup(!opt.isEmpty() ? opt.toUtf8().constData() : "");

    return cred;
}

void VncDesktopClient::gotXCutText(rfbClient *instance, const char *txt, int len)
{
    TRACE_ARG(txt << len);

    if (!txt || len < 1) return;
    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (self) self->clipboardText(QString::fromUtf8(txt, len));
}

static QVector<QRgb> usualColorTable()
{
    static QVector<QRgb> colors;
    if (colors.isEmpty()) {
        // red 3 bits, green 3 bits, blue 2 bits
        // make them maximum significant in 8bits this gives a colortable for 8bit true colors
        colors.resize(256);
        for (int i = 0; i < colors.size(); i++)
            colors[i] = qRgb((i & 0x07) << 5, (i & 0x38) << 2, i & 0xc0);
    }
    return colors;
}

void VncDesktopClient::gotCursorShape(rfbClient *instance, int hot_x, int hot_y, int width, int height, int bpp)
{
    //TRACE_ARG(x << y << width << height << bpp);

    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (!self) return;

    QImage image;
    switch (bpp) {
    case 1:
        image = QImage(instance->rcSource, width, height, bpp * width, QImage::Format_Indexed8);
        image.setColorTable(usualColorTable());
        break;
    case 2:
        image = QImage(instance->rcSource, width, height, bpp * width, QImage::Format_RGB16);
        break;
    case 4:
        image = QImage(instance->rcSource, width, height, bpp * width, QImage::Format_RGB32);
        break;
    default:
        if (vncLoggingEnabled) qWarning() << Q_FUNC_INFO << "Unexpected cursor shape BPP" << bpp;
        return;
    }
    QImage alpha(instance->rcMask, width, height, width, QImage::Format_Indexed8);
    alpha.setColorTable({ qRgb(255, 255, 255), qRgb(0, 0, 0) });

    QPixmap pixmap(QPixmap::fromImage(image));
    pixmap.setMask(QBitmap::fromImage(alpha));

    emit self->cursorChanged(QCursor(pixmap, hot_x, hot_y));
}

void VncDesktopClient::bell(rfbClient *instance)
{
    TRACE();

    auto self = reinterpret_cast<VncDesktopClient*>(::rfbClientGetClientData(instance, dataTag()));
    if (self) self->playBellSound(0, 0);
}
