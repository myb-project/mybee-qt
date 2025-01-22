#include <QTimer>
#include <QtDebug>

#include "SshChannel.h"
#include "SshSession.h"

//#define TRACE_SSHCHANNEL
#ifdef  TRACE_SSHCHANNEL
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshChannel::SshChannel(ssh::Session &sshSession, bool set_callbacks)
    : QIODevice()
    , lib_channel(sshSession)
    , eof_sent(false)
    , read0_bufsize(defaultBufSize)
    , read1_bufsize(defaultBufSize)
    , write_bufsize(defaultBufSize)
    , write_timer(nullptr)
{
    read0_buffer.reserve(read0_bufsize);
    write_buffer.reserve(write_bufsize);
    TRACE_ARG(read0_bufsize << write_bufsize);

    if (set_callbacks) {
        ::memset(&lib_callback, 0, sizeof(lib_callback));
        lib_callback.userdata                     = this;
        lib_callback.channel_data_function        = libDataCallback;
        lib_callback.channel_eof_function         = libEofCallback;
        lib_callback.channel_close_function       = libCloseCallback;
        lib_callback.channel_signal_function      = libSignalCallback;
        lib_callback.channel_exit_status_function = libExitStatusCallback;
        lib_callback.channel_exit_signal_function = libExitSignalCallback;
        ssh_callbacks_init(&lib_callback);
        if (::ssh_set_channel_callbacks(lib_channel.getCChannel(), &lib_callback) != SSH_OK) {
            qCritical() << Q_FUNC_INFO << "ssh_set_channel_callbacks:" << ::ssh_get_error(lib_channel.getCSession());
            return;
        }
    }
    if (!QIODevice::open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        qCritical() << Q_FUNC_INFO << "QIODevice::open() failed";
        return;
    }
}

SshChannel::~SshChannel()
{
    TRACE();
}

// This functions will be called when there is data available:
//  data - the data that has been read on the channel
//  len - the length of the data
//  is_stderr - 0 for stdout or 1 for stderr
// This function will not replace existing callbacks but set the new one atop of them!
//static
int SshChannel::libDataCallback(ssh_session session, ssh_channel channel,
                                void *data, uint32_t len, int is_stderr, void *userdata)
{
    TRACE_ARG(is_stderr << len);
    auto self = reinterpret_cast<SshChannel*>(userdata);
    if (!self || session != self->lib_channel.getCSession() || channel != self->lib_channel.getCChannel())
        return SSH_OK;
    return self->dataReceived(reinterpret_cast<const char *>(data), len, is_stderr != 0);
}

int SshChannel::dataReceived(const char *data, int len, bool std_err)
{
    if (isOpen()) {
        if (std_err && !readChannelCount()) std_err = false;
        int size = appendBuffer(data, len, std_err);
        if (size > 0) {
            if (readChannelCount())
                setCurrentReadChannel(std_err ? 1 : 0);
            emit readyRead();
            return size;
        }
        if (!std_err) eofReceived();
    }
    return 0;
}

int SshChannel::appendBuffer(const char *data, int len, bool std_err)
{
    TRACE_ARG(len << std_err);
    if (!data || len < 1) return 0;
    int size = std_err ? (read1_bufsize - read1_buffer.size()) : (read0_bufsize - read0_buffer.size());
    if (size < 1) return 0;
    if (size > len) size = len;
    if (std_err) read1_buffer.append(data, size);
    else read0_buffer.append(data, size);
    return size;
}

// This functions will be called when the channel has received an EOF.
//static
void SshChannel::libEofCallback(ssh_session session, ssh_channel channel, void *userdata)
{
    TRACE();
    auto self = reinterpret_cast<SshChannel*>(userdata);
    if (!self || session != self->lib_channel.getCSession() || channel != self->lib_channel.getCChannel())
        return;
    self->eofReceived();
}

void SshChannel::eofReceived()
{
    emit readChannelFinished();
}

// This functions will be called when the channel has been closed by remote.
//static
void SshChannel::libCloseCallback(ssh_session session, ssh_channel channel, void *userdata)
{
    TRACE();
    auto self = reinterpret_cast<SshChannel*>(userdata);
    if (!self || session != self->lib_channel.getCSession() || channel != self->lib_channel.getCChannel())
        return;
    self->closeReceived();
}

void SshChannel::closeReceived()
{
    if (isOpen()) QIODevice::close();
    clearBuffers();
    emit channelClosed();
}

// This functions will be called when a signal has been received:
//  signal - the signal name without the SIG prefix
//static
void SshChannel::libSignalCallback(ssh_session session, ssh_channel channel, const char *signal, void *userdata)
{
    TRACE_ARG(signal);
    auto self = reinterpret_cast<SshChannel*>(userdata);
    if (!self || session != self->lib_channel.getCSession() || channel != self->lib_channel.getCChannel())
        return;
    if (signal && *signal) self->signalReceived(QString(signal));
}


void SshChannel::signalReceived(const QString&)
{
}

// This functions will be called when an exit status has been received:
//  exit_status - exit status of the ran command.
//static
void SshChannel::libExitStatusCallback(ssh_session session, ssh_channel channel, int exit_status, void *userdata)
{
    TRACE_ARG(exit_status);
    auto self = reinterpret_cast<SshChannel*>(userdata);
    if (!self || session != self->lib_channel.getCSession() || channel != self->lib_channel.getCChannel())
        return;
    self->exitStatusReceived(exit_status);
}


void SshChannel::exitStatusReceived(int)
{
}

// This functions will be called when an exit signal has been received:
//  core - a boolean telling whether a core has been dumped or not
//  errmsg - the description of the exception
//  lang - the language of the description (format: RFC 3066)
//static
void SshChannel::libExitSignalCallback(ssh_session session, ssh_channel channel,
                                       const char *signal, int core,
                                       const char *errmsg, const char *lang, void *userdata)
{
    TRACE_ARG(signal << core << errmsg << lang);
    auto self = reinterpret_cast<SshChannel*>(userdata);
    if (!self || session != self->lib_channel.getCSession() || channel != self->lib_channel.getCChannel())
        return;
    QString signal_name;
    if (signal && *signal) signal_name = signal;
    QString err_msg;
    if (errmsg && *errmsg) err_msg = errmsg;
    QString err_lang;
    if (lang && *lang) err_lang = lang;
    self->exitSignalReceived(signal_name, core != 0, err_msg, err_lang);
}

void SshChannel::exitSignalReceived(const QString&, bool, const QString&, const QString&)
{
}

void SshChannel::setReadBufferSize(int size)
{
    TRACE_ARG(size);
    clearBuffers();
    read0_bufsize = qBound(defaultBufSize, size, maximumBufSize);
}

bool SshChannel::isChannelOpen() const
{
    if (!isOpen()) return false;
    auto cc = lib_channel.getCChannel();
    return (::ssh_channel_is_open(cc) != 0 && ::ssh_channel_is_closed(cc) == 0);
}

void SshChannel::sendEof()
{
    TRACE();
    if (!eof_sent && isChannelOpen()) {
        auto cc = lib_channel.getCChannel();
        if (::ssh_channel_send_eof(cc) == SSH_OK) eof_sent = true;
        else qWarning() << Q_FUNC_INFO << "ssh_channel_send_eof:" << ::ssh_get_error(cc);
    }
}

void SshChannel::clearBuffers()
{
    TRACE();
    read0_buffer.clear();
    read1_buffer.clear();
    write_buffer.clear();
    if (write_timer) write_timer->stop();
}

void SshChannel::writeDataChunk()
{
    TRACE();
    if (!write_timer || write_buffer.isEmpty()) return;
    if (isEofSent() || !isChannelOpen()) {
        write_timer->stop();
        write_buffer.clear();
        return;
    }
    auto cs = lib_channel.getCSession();
    int status = ::ssh_get_status(cs);
    if (status & (SSH_CLOSED | SSH_CLOSED_ERROR)) return;
    if ((status & SSH_WRITE_PENDING) == 0) {
        int max = qMin((int)write_buffer.size(), maxIOChunkSize);
        auto cc = lib_channel.getCChannel();
        int wb = ::ssh_channel_write(cc, write_buffer.constData(), max);
        if (wb < 1) {
            emit errorOccurred(QStringLiteral("ssh_channel_write: ") + ::ssh_get_error(cs));
            return;
        }
        write_buffer.remove(0, wb);
        emit bytesWritten(wb);
    }
    if (!write_buffer.isEmpty()) write_timer->start();
}

// Reimplemented from QIODevice

bool SshChannel::open(OpenMode mode)
{
    Q_UNUSED(mode);
    qWarning() << Q_FUNC_INFO << "Please don't use open() directly!";
    return false;
}

void SshChannel::close()
{
    TRACE();
    if (isOpen()) QIODevice::close();
    clearBuffers();
    if (isChannelOpen()) ::ssh_channel_close(lib_channel.getCChannel());
}

bool SshChannel::isSequential() const
{
    return true;
}

bool SshChannel::canReadLine() const
{
    TRACE();
    if (!QIODevice::canReadLine()) return false;
    if (readChannelCount() && currentReadChannel()) {
        return (read1_buffer.size() >= read1_bufsize || read1_buffer.contains('\n') ||
                (!read1_buffer.isEmpty() &&
                 (!isChannelOpen() || ::ssh_channel_is_eof(lib_channel.getCChannel()) != 0)));
    } else {
        return (read0_buffer.size() >= read0_bufsize || read0_buffer.contains('\n') ||
                (!read0_buffer.isEmpty() &&
                 (!isChannelOpen() || ::ssh_channel_is_eof(lib_channel.getCChannel()) != 0)));
    }
}

qint64 SshChannel::bytesAvailable() const
{
    bool std_err = (readChannelCount() && currentReadChannel());
    int size = std_err ? read1_buffer.size() : read0_buffer.size();
    size += QIODevice::bytesAvailable();
    TRACE_ARG(std_err << size);
    return size;
}

qint64 SshChannel::bytesToWrite() const
{
    TRACE_ARG(write_buffer.size());
    return write_buffer.size();
}

bool SshChannel::atEnd() const
{
    TRACE();
    if (!isOpen()) return true;
    bool std_err = (readChannelCount() && currentReadChannel());
    int size = std_err ? read1_buffer.size() : read0_buffer.size();
    return (!size && (!isChannelOpen() || ::ssh_channel_is_eof(lib_channel.getCChannel()) != 0));
}

qint64 SshChannel::readData(char *data, qint64 max_len)
{
    TRACE_ARG(data << max_len);
    if (!data || max_len < 1) return 0;
    bool std_err = (readChannelCount() && currentReadChannel());
    int size = std_err ? read1_buffer.size() : read0_buffer.size();
    TRACE_ARG(std_err << size);
    if (size > 0) {
        if (size > max_len) size = max_len;
        if (std_err) {
            ::memcpy(data, read1_buffer.constData(), size);
            read1_buffer.remove(0, size);
        } else {
            ::memcpy(data, read0_buffer.constData(), size);
            read0_buffer.remove(0, size);
        }
    }
    return size;
}

qint64 SshChannel::writeData(const char *data, qint64 len)
{
    TRACE_ARG(data << len);
    if (!data || len < 1 || (!isChannelOpen() || ::ssh_channel_is_eof(lib_channel.getCChannel()) != 0))
        return 0;

    if (openMode() == ReadOnly) {
        qWarning() << Q_FUNC_INFO << "Can't write to ReadOnly channel";
        return 0;
    }
    auto cs = lib_channel.getCSession();
    int status = ::ssh_get_status(cs);
    if (status & (SSH_CLOSED | SSH_CLOSED_ERROR)) return 0;
    if ((status & SSH_WRITE_PENDING) == 0) {
        auto cc = lib_channel.getCChannel();
        int wb = ::ssh_channel_write(cc, data, len);
        if (wb > 0) {
            emit bytesWritten(wb);
        } else {
            emit errorOccurred(QStringLiteral("ssh_channel_write: ") + ::ssh_get_error(cs));
        }
        return wb;
    }
    int max = write_bufsize - write_buffer.size();
    if (len > max) {
        emit errorOccurred(QStringLiteral("Output buffer overflow"));
        return 0;
    }
    write_buffer.append(data, len);
    if (!write_timer) {
        write_timer = new QTimer(this);
        write_timer->setInterval(100);
        write_timer->setSingleShot(true);
        connect(write_timer, &QTimer::timeout, this, &SshChannel::writeDataChunk);
    }
    write_timer->start();
    return len;
}
