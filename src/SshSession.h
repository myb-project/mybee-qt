#ifndef SSHSESSION_H
#define SSHSESSION_H

#include <QObject>
#include <QUrl>
#include <QSocketNotifier>
#include <QSharedPointer>
#include <QWeakPointer>

#include "libssh/libsshpp.hpp"
#include "libssh/callbacks.h"
#include "SshSettings.h"

class QTimer;
class SshChannel;
class SshChannelShell;
class SshChannelExec;
class SshChannelPort;

class SshSession;
typedef void (SshSession::*libSessionFunc)();

class SshSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int         logLevel READ logLevel     WRITE setLogLevel NOTIFY logLevelChanged FINAL)
    Q_PROPERTY(SshSettings settings READ settings     WRITE setSettings NOTIFY settingsChanged FINAL)
    Q_PROPERTY(bool        shareKey READ shareKey     WRITE setShareKey NOTIFY shareKeyChanged FINAL)
    Q_PROPERTY(QUrl          sshUrl READ sshUrl       NOTIFY sshUrlChanged FINAL)
    Q_PROPERTY(int            state READ state        NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool         running READ running      NOTIFY runningChanged FINAL)
    Q_PROPERTY(QString  hostAddress READ hostAddress  NOTIFY hostAddressChanged FINAL)
    Q_PROPERTY(QString  helloBanner READ helloBanner  NOTIFY helloBannerChanged FINAL)
    Q_PROPERTY(QString   pubkeyHash READ pubkeyHash   NOTIFY pubkeyHashChanged FINAL)
    Q_PROPERTY(int        knownHost READ knownHost    NOTIFY knownHostChanged FINAL)
    Q_PROPERTY(int     openChannels READ openChannels NOTIFY openChannelsChanged FINAL)
    Q_PROPERTY(QString    lastError READ lastError    NOTIFY lastErrorChanged FINAL)

public:
    static constexpr int const callAgainDelay = 100; // milliseconds by default
    static constexpr char const *authorizedKeys = ".ssh/authorized_keys";

    explicit SshSession(QObject *parent = nullptr);
    ~SshSession();

    enum LogLevel {
        LogLevelNone      = SSH_LOG_NOLOG,    // No logging at all
        LogLevelWarn      = SSH_LOG_WARNING,  // Only warnings
        LogLevelProtocol  = SSH_LOG_PROTOCOL, // High level protocol information
        LogLevelPacket    = SSH_LOG_PACKET,   // Lower level protocol infomations, packet level
        LogLevelFunctions = SSH_LOG_FUNCTIONS // Every function path
    };
    Q_ENUM(LogLevel)

    enum State {
        StateClosed,      // Not connected
        StateLookup,      // Resolve server hostAddress
        StateConnecting,  // Connecting
        StateServerCheck, // Checking the server
        StateUserAuth,    // User authentication
        StateEstablished, // Session is encrypted and authenticated
        StateReady,       // Session is ready for services: shell/exec/etc
        StateError,       // Error state
        StateDenied,      // Authentication failed
        StateTimeout,     // Connection timed out
        StateClosing      // Disconnecting
    };
    Q_ENUM(State)

    enum KnownHost {
        // The state has not been determined yet.
        KnownHostNone,
        // The server is known and has not changed.
        KnownHostOk,
        // The server is unknown. User should confirm the public key hash is correct.
        KnownHostUnknown,
        // The known host file does not exist. The host is thus unknown.
        // File will be created if host key is accepted.
        KnownHostCreated,
        // The server key has changed. Either you are under attack or the administrator changed the key.
        // You HAVE to warn the user about a possible attack.
        KnownHostUpdated,
        // The server provided a key of a certain type, while we had a different type recorded.
        // You HAVE to warn the user about a possible attack.
        KnownHostUpgraded
    };
    Q_ENUM(KnownHost)

    void setLogLevel(int level);
    bool setSettings(const SshSettings &settings);
    void setShareKey(bool enable);
    Q_INVOKABLE bool setSshUrl(const QUrl &url, const QString &key);

    bool isConnected() const;
    bool isEstablished() const;
    bool isReady() const;

    int logLevel() const { return log_level; }
    SshSettings settings() const { return ssh_settings; }
    bool shareKey() const { return share_key; }
    QUrl sshUrl() const { return ssh_url; }
    int state() const { return session_state; }
    bool running() const { return ssh_running; }
    QString hostAddress() const { return host_address; }
    QString helloBanner() const { return hello_banner; }
    QString pubkeyHash() const { return pubkey_hash; }
    int knownHost() const { return known_host; }
    int channels() const { return channel_list.size(); }
    int openChannels() const { return open_channels; }
    QString lastError() const { return last_error; }

public slots:
    void connectToHost();
    void connectToHost(const QString &user, const QString &host, int port = 0);
    void writeKnownHost();
    void giveAnswers(const QStringList &answers); // must match askQuestions's prompts
    void disconnectFromHost();
    void cancel();

signals:
    void logLevelChanged();
    void settingsChanged();
    void shareKeyChanged();
    void lastErrorChanged(const QString &text);
    void sshUrlChanged();
    void stateChanged();
    void runningChanged();
    void hostAddressChanged();
    void helloBannerChanged();
    void pubkeyHashChanged();
    void knownHostChanged();
    void askQuestions(const QString &info, const QStringList &prompts); // requre giveAnswers()
    void hostConnected();
    void openChannelsChanged();
    void hostDisconnected();

protected:
    friend class TextRender;
    friend class SshProcess;
    friend class DesktopView;
    QWeakPointer<SshChannelShell> createShell();
    QWeakPointer<SshChannelExec> createExec(const QString &command);
    QWeakPointer<SshChannelPort> createPort(const QString &host, quint16 port);
    void abortChannel(SshChannel *channel);

private:
    void cleanUp(bool abort = false);
    bool closeChannel(SshChannel *channel = nullptr); // null to close all channels
    void setRunning(bool on);
    void setLastError(const QString &text);
    void setConnectTimer(bool enable);
    bool isConnectTimer() const;
    void callLater(libSessionFunc func, int msec = 0);
    bool isCallLater() const;
    void setState(State state);
    void lookupHost();
    bool setSocketNotifiers(bool on);
    bool isSocketNotifiers() const;
    void onSocketActivated(QSocketDescriptor sd, QSocketNotifier::Type type);
    void checkPollFlags();
    void libEventDoPoll();
    void libConnect();
    void libSetBlocking();
    void libServerPublickey();
    void libServerKnown();
    void libUserAuthNone();
    void iterateUserAuth();
    void libUserAuthTryPublickey();
    void libUserAuthPublickey();
    void libUserAuthKbdint();
    void libUserAuthPassword();
    void setOpenChannels();
    void onPullDone();
    void onChannelClosed();
    void sendPublicKey();
    void emitHostDisconnected();
    static int libEventCallback(socket_t fd, int revents, void *userdata);

    LogLevel log_level;
    SshSettings ssh_settings;
    bool share_key;

    mutable ssh::Session lib_session;
    ssh_event lib_main_event;

    int lookup_id;
    State session_state;
    QSocketNotifier *read_notifier;
    QSocketNotifier *write_notifier;

    QString pubkey_hash;
    KnownHost known_host;
    int user_auth;
    struct ssh_key_struct *lib_ssh_key;
    QTimer *connect_timer;
    QTimer *later_timer;

    QUrl ssh_url;
    bool ssh_running;
    QString host_address;
    QString hello_banner;
    int open_channels;
    QString last_error;

    QList<QSharedPointer<SshChannel>> channel_list;
};

#endif // SSHSESSION_H
