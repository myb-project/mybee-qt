#ifndef SSHCHANNEL_H
#define SSHCHANNEL_H

#include <QIODevice>

#include "libssh/libsshpp.hpp"
#include "libssh/callbacks.h"

class QTimer;
class SshSession;

class SshChannel : public QIODevice
{
    Q_OBJECT

public:
    static constexpr int const maximumBufSize = 1024 * 1024;
    static constexpr int const defaultBufSize = 65536;
    static constexpr int const maxIOChunkSize = 8192;

    virtual ~SshChannel() override;

    void setReadBufferSize(int size); // in bytes
    bool isChannelOpen() const;
    bool isEofRecv() const;
    virtual void sendEof();
    bool isEofSent() const { return eof_sent; }

    // Reimplemented from QIODevice
    bool open(OpenMode mode) override;
    bool isSequential() const override;
    bool canReadLine() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    bool atEnd() const override;
    void close() override;

signals:
    void errorOccurred(const QString &text);
    void channelClosed();

protected:
    explicit SshChannel(ssh::Session &sshSession, bool set_callbacks = true);
    mutable ssh::Channel lib_channel;

    virtual int dataReceived(const char *data, int len, bool std_err); // append to read_buffer
    virtual void eofReceived();   // emit readChannelFinished() by default
    virtual void closeReceived(); // call QIODevice::close() and emit channelClosed() by default
    virtual void signalReceived(const QString &signal_name); // do nothing
    virtual void exitStatusReceived(int exit_status);        // do nothing
    virtual void exitSignalReceived(const QString &signal_name, bool core_dump,
                                    const QString &err_msg, const QString &err_lang); // rfc3066
    int appendBuffer(const char *data, int len, bool std_err = false);
    void clearBuffers();

    // Reimplemented from QIODevice
    qint64 readData(char *data, qint64 max_len) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    void writeDataChunk();

    static int libDataCallback(ssh_session, ssh_channel, void *data, uint32_t len, int is_stderr, void *userdata);
    static void libEofCallback(ssh_session, ssh_channel, void *userdata);
    static void libCloseCallback(ssh_session, ssh_channel, void *userdata);
    static void libSignalCallback(ssh_session, ssh_channel, const char *signal, void *userdata);
    static void libExitStatusCallback(ssh_session, ssh_channel, int exit_status, void *userdata);
    static void libExitSignalCallback(ssh_session, ssh_channel, const char *signal, int core,
                                      const char *errmsg, const char *lang, void *userdata);
    struct ssh_channel_callbacks_struct lib_callback;

    bool eof_sent;
    int read0_bufsize;
    int read1_bufsize;
    int write_bufsize;
    QByteArray read0_buffer;
    QByteArray read1_buffer;
    QByteArray write_buffer;
    QTimer *write_timer;
};

#endif // SSHCHANNEL_H
