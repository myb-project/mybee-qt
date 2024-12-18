#include <QTimer>
#include <QFileInfo>
#include <QtDebug>

#include <fcntl.h>

#include "SshChannelFtp.h"
#include "SshSession.h"

//#define TRACE_SSHCHANNELFTP
#ifdef  TRACE_SSHCHANNELFTP
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshChannelFtp::SshChannelFtp(ssh::Session &sshSession, const QString &remotePath)
    : SshChannel(sshSession, false) // don't use default callbacks!
    , remote_path(remotePath)
    , lib_session(nullptr)
    , later_timer(nullptr)
    , lib_file(nullptr)
    , lib_aio(nullptr)
{
    TRACE();
    if (remote_path.isEmpty()) {
        qCritical() << Q_FUNC_INFO << "An empty remote path specified";
        return;
    }
    lib_session = ::sftp_new_channel(lib_channel.getCSession(), lib_channel.getCChannel());
    if (!lib_session) {
        qCritical() << Q_FUNC_INFO << "sftp_new_channel:" << lastError();
        return;
    }
    setReadBufferSize(maximumBufSize);
    callLater(&SshChannelFtp::libOpenSession);
}

SshChannelFtp::~SshChannelFtp()
{
    TRACE();
    if (lib_session) {
        if (lib_aio) ::sftp_aio_free(lib_aio);
        if (lib_file) ::sftp_close(lib_file);
        ::sftp_free(lib_session);
    }
}

QString SshChannelFtp::lastError() const
{
    int code = ::sftp_get_error(lib_session);
    switch (code) {
    case SSH_FX_OK:
        return QStringLiteral("No error");
    case SSH_FX_EOF:
        return QStringLiteral("End-of-file encountered");
    case SSH_FX_NO_SUCH_FILE:
        return QStringLiteral("File doesn't exist");
    case SSH_FX_PERMISSION_DENIED:
        return QStringLiteral("Permission denied");
    case SSH_FX_FAILURE:
        return QStringLiteral("Generic failure");
    case SSH_FX_BAD_MESSAGE:
        return QStringLiteral("Garbage received from server");
    case SSH_FX_NO_CONNECTION:
        return QStringLiteral("No connection has been set up");
    case SSH_FX_CONNECTION_LOST:
        return QStringLiteral("There was a connection, but we lost it");
    case SSH_FX_OP_UNSUPPORTED:
        return QStringLiteral("Operation not supported by the server");
    case SSH_FX_INVALID_HANDLE:
        return QStringLiteral("Invalid file handle");
    case SSH_FX_NO_SUCH_PATH:
        return QStringLiteral("No such file or directory path exists");
    case SSH_FX_FILE_ALREADY_EXISTS:
        return QStringLiteral("An attempt to create an already existing file or directory");
    case SSH_FX_WRITE_PROTECT:
        return QStringLiteral("Write-protected filesystem");
    case SSH_FX_NO_MEDIA:
        return QStringLiteral("No media in remote drive");
    }
    const char *cp = ::ssh_get_error(lib_channel.getCSession());
    return (cp && *cp) ? cp : QString("Unknown error %1").arg(code);
}

void SshChannelFtp::callLater(libFtpFunc func, int msec)
{
    //TRACE_ARG(msec);
    if (!func) {
        if (later_timer) later_timer->stop();
        return;
    }
    if (!later_timer) {
        later_timer = new QTimer(this);
        later_timer->setSingleShot(true);
    } else later_timer->disconnect();
    connect(later_timer, &QTimer::timeout, this, [this,func]() { (this->*func)(); });
    later_timer->start(qBound(0, msec, 1000));
}

void SshChannelFtp::libOpenSession()
{
    TRACE();
    int rc = ::ssh_channel_open_session(lib_channel.getCChannel());
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelFtp::libOpenSession, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_open_session: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    callLater(&SshChannelFtp::libRequestFtp);
}

void SshChannelFtp::libRequestFtp()
{
    TRACE();
    int rc = ::ssh_channel_request_sftp(lib_channel.getCChannel());
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelFtp::libRequestFtp, SshSession::callAgainDelay);
        return;
    }
    if (rc != SSH_OK) {
        emit errorOccurred(QStringLiteral("ssh_channel_request_sftp: ") + ::ssh_get_error(lib_channel.getCSession()));
        return;
    }
    if (::sftp_init(lib_session) != SSH_OK) {
        emit errorOccurred(QStringLiteral("sftp_init: ") + lastError());
        return;
    }
    callLater(nullptr);
    emit channelOpened();
}

void SshChannelFtp::pull()
{
    TRACE();
    if (lib_file || libOpenFile(O_RDWR | O_CREAT)) {
        ::sftp_file_set_nonblocking(lib_file);
        libAsyncReadBegin();
        return;
    }
    int rc = ::sftp_get_error(lib_session);
    if (rc == SSH_FX_NO_SUCH_FILE || rc == SSH_FX_NO_SUCH_PATH) {
        Q_ASSERT(!remote_path.isEmpty());
        QString path =  QFileInfo(remote_path).path();
        if (!path.isEmpty() && path != ".") {
            mkdir_list = path.split('/', Qt::SkipEmptyParts);
            mkdir_path.clear();
            if (!mkdir_list.isEmpty()) {
                callLater(&SshChannelFtp::makeDirPath);
                return;
            }
        }
    }
    emit errorOccurred(QStringLiteral("sftp_open: ") + lastError());
}

void SshChannelFtp::makeDirPath()
{
    TRACE_ARG(mkdir_list << mkdir_path);
    if (mkdir_list.isEmpty()) {
        emit readyRead();
        return;
    }
    if (!mkdir_path.isEmpty() || QFileInfo(remote_path).isAbsolute())
        mkdir_path += '/';
    mkdir_path += mkdir_list.takeFirst();
    if (::sftp_mkdir(lib_session, qPrintable(mkdir_path), 0700) < 0) {
        emit errorOccurred(QStringLiteral("sftp_mkdir: ") + lastError());
        return;
    }
    callLater(&SshChannelFtp::makeDirPath);
}

bool SshChannelFtp::libOpenFile(int mode, mode_t perm)
{
    TRACE_ARG(mode << perm);
    if (!lib_file) {
        Q_ASSERT(!remote_path.isEmpty());
        if (lib_aio) {
            ::sftp_aio_free(lib_aio);
            lib_aio = nullptr;
        }
        lib_file = ::sftp_open(lib_session, qPrintable(remote_path), mode, perm);
        if (!lib_file) {
            clearBuffers();
            return false;
        }
    }
    return true;
}

void SshChannelFtp::libAsyncReadBegin()
{
    TRACE();
    int rc = ::sftp_aio_begin_read(lib_file, maxIOChunkSize, &lib_aio);
    if (rc == SSH_ERROR) {
        clearBuffers();
        emit errorOccurred(QStringLiteral("sftp_aio_begin_read: ") + lastError());
        return;
    }
    callLater(&SshChannelFtp::libAsyncRead);
}

void SshChannelFtp::libAsyncRead()
{
    TRACE();
    if (!lib_aio) return;
    char buf[maxIOChunkSize];
    int rc = ::sftp_aio_wait_read(&lib_aio, buf, maxIOChunkSize);
    if (rc == SSH_AGAIN) {
        callLater(&SshChannelFtp::libAsyncRead, SshSession::callAgainDelay);
        return;
    }
    lib_aio = nullptr;
    if (rc == SSH_ERROR) {
        clearBuffers();
        emit errorOccurred(QStringLiteral("sftp_aio_wait_read: ") + lastError());
        return;
    }
    if (rc && appendBuffer(buf, rc)) {
        libAsyncReadBegin();
        return;
    }
    emit readyRead();
}

void SshChannelFtp::push(const QByteArray &data, bool append)
{
    TRACE_ARG(data.size());
    if (data.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Empty data";
        return;
    }
    if (!lib_file && !libOpenFile(O_RDWR | O_CREAT)) {
        emit errorOccurred(QStringLiteral("sftp_open: ") + lastError());
        return;
    }
    ::sftp_file_set_blocking(lib_file);
    if (append) {
        int pos = ::sftp_tell(lib_file);
        if (pos < 0) {
            emit errorOccurred(QStringLiteral("sftp_tell: ") + lastError());
            return;
        }
        if (::sftp_seek(lib_file, pos) < 0) {
            emit errorOccurred(QStringLiteral("sftp_seek: ") + lastError());
            return;
        }
    } else ::sftp_rewind(lib_file);
    if (::sftp_write(lib_file, data.constData(), data.size()) < 0) {
        emit errorOccurred(QStringLiteral("sftp_write: ") + lastError());
        return;
    }
}

void SshChannelFtp::close()
{
    TRACE();
    if (lib_file) {
        if (lib_aio) {
            ::sftp_aio_free(lib_aio);
            lib_aio = nullptr;
        }
        if (::sftp_close(lib_file) != SSH_OK)
            qWarning() << Q_FUNC_INFO << "sftp_close:" << lastError();
        lib_file = nullptr;
    }
    SshChannel::close();
}
