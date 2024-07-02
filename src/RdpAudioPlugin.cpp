#include <QAudioOutput>
#include <QAudioFormat>
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#endif

#include "RdpAudioPlugin.h"

int RdpAudioPlugin::create(PFREERDP_RDPSND_DEVICE_ENTRY_POINTS pEntryPoints)
{
    auto plugin = new RdpAudioPlugin;
    plugin->device.Open = open;
    plugin->device.FormatSupported = isFormatSupported;
    plugin->device.GetVolume = getVolume;
    plugin->device.SetVolume = setVolume;
    plugin->device.Play = play;
    plugin->device.Start = start;
    plugin->device.Close = close;
    plugin->device.Free = free;

    pEntryPoints->pRegisterRdpsndDevice(pEntryPoints->rdpsnd,
                                        reinterpret_cast<rdpsndDevicePlugin*>(plugin));
    return 0;
}

void RdpAudioPlugin::free(rdpsndDevicePlugin *device)
{
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (self) delete self;
}

BOOL RdpAudioPlugin::open(rdpsndDevicePlugin *device, const AUDIO_FORMAT *format, UINT32 latency)
{
    Q_UNUSED(latency);
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (!self) return false;
    self->resetAudioOut(self->toQtFormat(format), format->nAvgBytesPerSec);
    self->outDevice = self->audioOut->start();
    return true;
}

void RdpAudioPlugin::close(rdpsndDevicePlugin *device)
{
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (self) self->audioOut->stop();
}

BOOL RdpAudioPlugin::isFormatSupported(rdpsndDevicePlugin *device, const AUDIO_FORMAT *format)
{
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (!self) return false;
    QAudioFormat fmt = self->toQtFormat(format);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return fmt.isValid();
#else
    return QAudioDevice(QMediaDevices::defaultAudioOutput()).isFormatSupported(fmt);
#endif
}

BOOL RdpAudioPlugin::setFormat(rdpsndDevicePlugin *device, const AUDIO_FORMAT *format, int latency)
{
    Q_UNUSED(latency);
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (!self) return false;
    self->resetAudioOut(self->toQtFormat(format), format->nAvgBytesPerSec);
    return true;
}

UINT32 RdpAudioPlugin::getVolume(rdpsndDevicePlugin *device)
{
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (!self) return 0;
    quint32 volume = qRound(self->audioOut->volume() * 0xFFFF);
    return (volume << 16) | (volume & 0xFFFF);
}

BOOL RdpAudioPlugin::setVolume(rdpsndDevicePlugin *device, UINT32 value)
{
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (!self) return false;
    qreal left = qreal(value & 0xFFFF) / 0xFFFF;
    qreal right = qreal((value >> 16) & 0xFFFF) / 0xFFFF;
    self->audioOut->setVolume((left + right) / 2);
    return true;
}

UINT RdpAudioPlugin::play(rdpsndDevicePlugin *device, const BYTE *data, size_t size)
{
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    return (self && self->outDevice) ? self->outDevice->write((const char *)data, size) : 0;
}

void RdpAudioPlugin::start(rdpsndDevicePlugin *device)
{
    auto self = reinterpret_cast<RdpAudioPlugin*>(device);
    if (self) self->audioOut->reset();
}

QAudioFormat RdpAudioPlugin::toQtFormat(const AUDIO_FORMAT *in) const
{
    if (!in) return QAudioFormat();

    QAudioFormat out;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("audio/pcm");
    out.setSampleSize(in->wBitsPerSample);
    out.setSampleType(QAudioFormat::UnSignedInt);
#else
    out.setSampleFormat(QAudioFormat::UInt8);
#endif
    out.setSampleRate(in->nSamplesPerSec);
    out.setChannelCount(in->nChannels);
    return out;
}

void RdpAudioPlugin::resetAudioOut(const QAudioFormat &format, int avgBytesPerSec)
{
    delete audioOut;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    audioOut = new QAudioOutput(format);
#else
    audioOut = new QAudioSink(format);
#endif
    // buffer size worth of 10 seconds should be enough
    audioOut->setBufferSize(avgBytesPerSec * 10);
}

RdpAudioPlugin::RdpAudioPlugin()
{
    ::memset(&device, 0, sizeof(device));
    resetAudioOut(QAudioFormat(), 0);
}

RdpAudioPlugin::~RdpAudioPlugin()
{
    delete audioOut;
}
