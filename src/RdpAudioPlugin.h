#ifndef RDPAUDIOPLUGIN_H
#define RDPAUDIOPLUGIN_H

#include <QPointer>
#include <freerdp/client/rdpsnd.h>

class QIODevice;
class QAudioOutput;
class QAudioFormat;
class QAudioSink;

/**
 * The RdpAudioPlugin class lets FreeRDP to play audio through Qt.
 *
 * The class implements rdpsnd's plugin interface which will allow plugging it
 * into FreeRDP. This class acts as wrapper around QAudioOutput.
 */
class RdpAudioPlugin
{
public:
    static int create(PFREERDP_RDPSND_DEVICE_ENTRY_POINTS pEntryPoints);

    // interface "functions" for FreeRDP
    static void free(rdpsndDevicePlugin *device);
    static BOOL open(rdpsndDevicePlugin *device, const AUDIO_FORMAT *format, UINT32 latency);
    static void close(rdpsndDevicePlugin *device);
    static BOOL isFormatSupported(rdpsndDevicePlugin *device, const AUDIO_FORMAT *format);
    static BOOL setFormat(rdpsndDevicePlugin *device, const AUDIO_FORMAT *format, int latency);
    static UINT32 getVolume(rdpsndDevicePlugin *device);
    static BOOL setVolume(rdpsndDevicePlugin *device, UINT32 value);
    static UINT play(rdpsndDevicePlugin *device, const BYTE *data, size_t size);
    static void start(rdpsndDevicePlugin *device);

private:
    QAudioFormat toQtFormat(const AUDIO_FORMAT *in) const;
    void resetAudioOut(const QAudioFormat &format, int avgBytesPerSec);

    RdpAudioPlugin();
    ~RdpAudioPlugin();

    rdpsndDevicePlugin device;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPointer<QAudioOutput> audioOut;
#else
    QPointer<QAudioSink> audioOut;
#endif
    QPointer<QIODevice> outDevice;
};

#endif // RDPAUDIOPLUGIN_H
