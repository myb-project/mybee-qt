#include <QTimer>
#include <QHostInfo>
#include <QFileInfo>
#include <QFile>
#include <QMetaMethod>
#include <QtDebug>

#include <poll.h>

#include "SshSession.h"
#include "SshChannelShell.h"
#include "SshChannelExec.h"
#include "SshChannelFtp.h"
#include "TextRender.h"

//#define TRACE_SSHSESSION
#ifdef  TRACE_SSHSESSION
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshSession::SshSession(QObject *parent)
    : QObject(parent)
    , share_key(false)
    , lib_main_event(nullptr)
    , lookup_id(-1)
    , session_state(StateClosed)
    , read_notifier(nullptr)
    , write_notifier(nullptr)
    , known_host(KnownHostNone)
    , user_auth(0)
    , lib_ssh_key(nullptr)
    , connect_timer(nullptr)
    , later_timer(nullptr)
    , ssh_running(false)
    , ssh_established(false)
    , open_channels(0)
{
    qRegisterMetaType<SshSettings>("SshSettings");

    int ll = qEnvironmentVariable("LIBSSH_DEBUG").toInt();
    log_level = static_cast<LogLevel>(qBound(int(LogLevelNone), ll, int(LogLevelFunctions)));
    TRACE_ARG(log_level);
}

SshSession::~SshSession()
{
    TRACE();
    cleanUp(true);
}

void SshSession::cleanUp(bool abort)
{
    TRACE_ARG(abort);
    if (lookup_id != -1) {
        QHostInfo::abortHostLookup(lookup_id);
        lookup_id = -1;
    }
    if (isConnectTimer()) setConnectTimer(false);
    if (isCallLater()) callLater(nullptr);
    if (isSocketNotifiers()) setSocketNotifiers(false);
    if (channels()) closeChannel();

    pubkey_hash.clear();
    known_host = KnownHostNone;
    user_auth = 0;
    if (lib_ssh_key) {
        ::ssh_key_free(lib_ssh_key);
        lib_ssh_key = nullptr;
    }
    if (abort) {
        auto cs = lib_session.getCSession();
        if (::ssh_is_connected(cs) != 0) ::ssh_silent_disconnect(cs);
    } else {
        if (ssh_established) setEstablished(false);
        if (ssh_running) setRunning(false);
    }
}

bool SshSession::closeChannel(SshChannel *channel)
{
    TRACE_ARG(channel_list.size() << channel);
    if (!channel_list.isEmpty()) {
        for (int i = 0; i < channel_list.size(); i++) {
            if (!channel) {
                // we initiated the closure of the channel
                channel_list.at(i)->sendEof();
                channel_list.at(i)->close();
            } else if (channel == channel_list.at(i).data()) {
                // the channel is already closed by the remote side
                channel_list.removeAt(i);
                return true;
            }
        }
        if (channel) {
            qWarning() << Q_FUNC_INFO << "No such channel:" << channel;
            return false;
        }
        channel_list.clear();
    }
    return true;
}

//static
int SshSession::libEventCallback(socket_t fd, int revents, void *userdata)
{
    if (revents == (POLLIN | POLLOUT)) {
        TRACE_ARG("Connected:" << fd << revents);
        if (fd == SSH_INVALID_SOCKET) {
            qCritical() << Q_FUNC_INFO << "Invalid socket descriptor!";
            return SSH_ERROR;
        }
        auto self = reinterpret_cast<SshSession*>(userdata);
        if (!self) {
            qCritical() << Q_FUNC_INFO << "Invalid userdata parameter!";
            return SSH_ERROR;
        }
        // The socket is connected now, so turn off nonblocking mode and remove this watcher
        if (::ssh_is_blocking(self->lib_session.getCSession()) == 0)
            self->callLater(&SshSession::libSetBlocking);
    }
    return SSH_OK;
}

void SshSession::libSetBlocking()
{
    TRACE();
    auto cs = lib_session.getCSession();
    auto fd = ::ssh_get_fd(cs);
    if (fd == SSH_INVALID_SOCKET) {
        setState(StateError);
        return;
    }
    ::ssh_set_blocking(cs, 1);
    if (lib_main_event) ::ssh_event_remove_fd(lib_main_event, fd);
    callLater(&SshSession::libConnect);
    emit hostConnected();
}

void SshSession::setConnectTimer(bool enable)
{
    TRACE_ARG(enable);
    if (!enable) {
        if (isConnectTimer()) connect_timer->stop();
        return;
    }
    if (!connect_timer) {
        connect_timer = new QTimer(this);
        connect_timer->setSingleShot(true);
        connect_timer->callOnTimeout([this]() {
            TRACE_ARG(session_state);
            if (session_state == StateConnecting)
                setState(StateTimeout);
        });
    }
    int msec = ssh_settings.timeout() * 1000 + 100; // make milliseconds with slight spare
    connect_timer->start(qBound(1000, msec, 10000));
}

bool SshSession::isConnectTimer() const
{
    return (connect_timer && connect_timer->isActive());
}

void SshSession::callLater(libSessionFunc func, int msec)
{
    //TRACE_ARG(msec);
    if (!func) {
        if (isCallLater()) later_timer->stop();
        return;
    }
    if (!later_timer) {
        later_timer = new QTimer(this);
        later_timer->setSingleShot(true);
    } else later_timer->disconnect();
    connect(later_timer, &QTimer::timeout, this, [this,func]() { (this->*func)(); });
    later_timer->start(qBound(0, msec, 1000));
}

bool SshSession::isCallLater() const
{
    return (later_timer && later_timer->isActive());
}

void SshSession::setSettings(const SshSettings &settings)
{
    TRACE_ARG(settings);
    if (session_state != StateClosed) {
        qWarning() << Q_FUNC_INFO << "Bad state, disconnect first!";
        return;
    }
    if (!settings.isValid()) {
        qWarning() << Q_FUNC_INFO << "The settings is invalid";
        return;
    }
    if (settings != ssh_settings) {
        ssh_settings = settings;
        emit settingsChanged();
    }
}

void SshSession::setShareKey(bool enable)
{
    TRACE_ARG(enable);
    if (enable != share_key) {
        share_key = enable;
        emit shareKeyChanged();
    }
}

void SshSession::setLogLevel(int level)
{
    TRACE_ARG(level);
    if (session_state != StateClosed) {
        qWarning() << Q_FUNC_INFO << "Bad state, disconnect first!";
        return;
    }
    LogLevel ll = static_cast<LogLevel>(qBound(int(LogLevelNone), level, int(LogLevelFunctions)));
    if (ll != log_level) {
        log_level = ll;
        emit logLevelChanged();
    }
}

void SshSession::setRunning(bool on)
{
    TRACE_ARG(on);
    if (on != ssh_running) {
        ssh_running = on;
        emit runningChanged();
    }
}

void SshSession::setEstablished(bool on)
{
    TRACE_ARG(on);
    if (on != ssh_established) {
        ssh_established = on;
        emit establishedChanged();
    }
}

void SshSession::setLastError(const QString &text)
{
    TRACE_ARG(text);
    if (text != last_error) {
        last_error = text;
        emit lastErrorChanged();
    }
}

void SshSession::connectToHost()
{
    TRACE();
    if (session_state != StateClosed) {
        qWarning() << Q_FUNC_INFO << "Bad state, disconnect first!";
        return;
    }
    if (!ssh_settings.isValid()) {
        qWarning() << Q_FUNC_INFO << "The settings is invalid";
        return;
    }
    cleanUp();
    auto cs = lib_session.getCSession();
    long opt = log_level;
    if (::ssh_options_set(cs, SSH_OPTIONS_LOG_VERBOSITY, &opt) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option LogLevel failed";
        return;
    }
    opt = ssh_settings.port();
    if (::ssh_options_set(cs, SSH_OPTIONS_PORT, &opt) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option Port failed";
        return;
    }
    if (::ssh_options_set(cs, SSH_OPTIONS_USER, qPrintable(ssh_settings.user())) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option User failed";
        return;
    }
    if (::ssh_options_set(cs, SSH_OPTIONS_HOST, qPrintable(ssh_settings.host())) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option Host failed";
        return;
    }
    opt = ssh_settings.timeout();
    if (::ssh_options_set(cs, SSH_OPTIONS_TIMEOUT, &opt) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option Timeout failed";
        return;
    }
    opt = 1;
    if (::ssh_options_set(cs, SSH_OPTIONS_NODELAY, &opt) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option NoDelay failed";
        return;
    }
    if (::ssh_options_set(cs, SSH_OPTIONS_CIPHERS_C_S, "chacha20-poly1305@openssh.com,aes256-ctr") < 0) {
        qWarning() << Q_FUNC_INFO << "Set option SSH_OPTIONS_CIPHERS_C_S failed";
        return;
    }
    if (::ssh_options_set(cs, SSH_OPTIONS_CIPHERS_S_C, "chacha20-poly1305@openssh.com,aes256-ctr") < 0) {
        qWarning() << Q_FUNC_INFO << "Set option SSH_OPTIONS_CIPHERS_S_C failed";
        return;
    }
    if (!ssh_settings.privateKey().isEmpty() &&
        ::ssh_options_set(cs, SSH_OPTIONS_SSH_DIR, qPrintable(QFileInfo(ssh_settings.privateKey()).path())) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option SshDir failed";
        return;
    }
    if (::ssh_options_parse_config(cs, nullptr) < 0) {
        qWarning() << Q_FUNC_INFO << "Set option Config failed";
        return;
    }
    setShareKey(false);
    setLastError(QString());
    QUrl url(ssh_settings.toString());
    if (url != ssh_url) {
        ssh_url = url;
        emit urlChanged();
    }
    if (!QHostAddress().setAddress(ssh_settings.host())) {
        setState(StateLookup);
        return;
    }
    if (ssh_settings.host() != host_address) {
        host_address = ssh_settings.host();
        emit hostAddressChanged();
    }
    setState(StateConnecting);
}

void SshSession::connectToHost(const QString &user, const QString &host, int port)
{
    TRACE();
    setSettings(SshSettings(user, host, port));
    connectToHost();
}

void SshSession::disconnectFromHost()
{
    TRACE();
    if (session_state != StateClosing && session_state != StateClosed)
        setState(StateClosing);
}

void SshSession::cancel()
{
    TRACE();
    setState(StateClosed);
}

bool SshSession::isConnected() const
{
    return (session_state != StateClosed && ::ssh_get_fd(lib_session.getCSession()) != SSH_INVALID_SOCKET);
}

bool SshSession::isEstablished() const
{
    return (session_state == StateEstablished && ::ssh_is_connected(lib_session.getCSession()) != 0);
}

bool SshSession::isReady() const
{
    return (session_state == StateReady && ::ssh_is_connected(lib_session.getCSession()) != 0);
}

void SshSession::setState(State state)
{
    TRACE_ARG(session_state << state);
    if (state == session_state) return;
    State prev_state = session_state;
    session_state = state;

    setConnectTimer(session_state == StateConnecting);
    if (isCallLater()) callLater(nullptr);

    switch (session_state) {
    case StateLookup:
        callLater(&SshSession::lookupHost);
        break;
    case StateConnecting:
        ::ssh_set_blocking(lib_session.getCSession(), 0);
        callLater(&SshSession::libConnect);
        break;
    case StateServerCheck:
        callLater(&SshSession::libServerPublickey);
        break;
    case StateUserAuth:
        callLater(&SshSession::libUserAuthNone);
        break;
    case StateEstablished:
        setEstablished(true);
        callLater(&SshSession::sendPublicKey);
        break;
    case StateReady:
        break;
    case StateError:
        if (last_error.isEmpty()) {
            auto cs = lib_session.getCSession();
            setLastError(::ssh_get_error_code(cs) != SSH_NO_ERROR ?
                         ::ssh_get_error(cs) : QStringLiteral("Unknown error"));
        }
    case StateDenied:
        if (session_state == StateDenied && last_error.isEmpty()) {
            setLastError(QStringLiteral("Authentication failed"));
        }
    case StateTimeout:
        if (session_state == StateTimeout && last_error.isEmpty()) {
            setLastError(QString("Connection timed out (max %1 sec)").arg(ssh_settings.timeout()));
        }
    case StateClosed:
        cleanUp();
    case StateClosing:
        if (prev_state == StateEstablished) {
            if (channels()) closeChannel();
            if (open_channels) {
                open_channels = 0;
                emit openChannelsChanged();
            }
        }
        if (::ssh_is_connected(lib_session.getCSession())) {
            if (isSocketNotifiers()) setSocketNotifiers(false);
            ::ssh_disconnect(lib_session.getCSession());
        }
        if (session_state == StateClosing) callLater(&SshSession::cancel);
        else if (session_state == StateClosed) callLater(&SshSession::emitHostDisconnected);
        break;
    }
    emit stateChanged();
}

void SshSession::lookupHost()
{
    TRACE();
    if (lookup_id != -1) QHostInfo::abortHostLookup(lookup_id);
    lookup_id = QHostInfo::lookupHost(ssh_settings.host(), this, [this](const QHostInfo &info) {
        lookup_id = -1;
        if (info.error() != QHostInfo::NoError) {
            setLastError(info.errorString());
            setState(StateError);
            return;
        }
        if (info.addresses().isEmpty()) {
            setLastError(QStringLiteral("Host IP address not found"));
            setState(StateError);
            return;
        }
        QString ip_addr = info.addresses().constFirst().toString();
        if (ip_addr.isEmpty()) ip_addr = "0.0.0.0";
        else ::ssh_options_set(lib_session.getCSession(), SSH_OPTIONS_HOST, qPrintable(ip_addr));
        QString hst_addr = ssh_settings.host() + " [" + ip_addr + ']';
        if (hst_addr != host_address) {
            host_address = hst_addr;
            emit hostAddressChanged();
        }
        setState(StateConnecting);
    });
}

bool SshSession::setSocketNotifiers(bool on)
{
    TRACE_ARG(on);
    auto cs = lib_session.getCSession();
    auto fd = ::ssh_get_fd(cs);
    if (read_notifier) {
        read_notifier->setEnabled(false);
        read_notifier->deleteLater();
        read_notifier = nullptr;
    }
    if (write_notifier) {
        write_notifier->setEnabled(false);
        write_notifier->deleteLater();
        write_notifier = nullptr;
    }
    if (lib_main_event) {
        if (fd != SSH_INVALID_SOCKET) ::ssh_event_remove_fd(lib_main_event, fd);
        ::ssh_event_remove_session(lib_main_event, cs);
        ::ssh_event_free(lib_main_event);
        lib_main_event = nullptr;
    }
    if (!on || fd == SSH_INVALID_SOCKET) return false;

    if (!lib_main_event) {
        lib_main_event = ::ssh_event_new();
        if (::ssh_event_add_fd(lib_main_event, fd, POLLIN | POLLOUT, libEventCallback, this) != SSH_OK) {
            ::ssh_event_free(lib_main_event);
            lib_main_event = nullptr;
            return false;
        }
        if (::ssh_event_add_session(lib_main_event, cs) != SSH_OK) {
            ::ssh_event_remove_fd(lib_main_event, fd);
            ::ssh_event_free(lib_main_event);
            lib_main_event = nullptr;
            return false;
        }
    }
    read_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    read_notifier->setEnabled(false);
    connect(read_notifier, &QSocketNotifier::activated, this, &SshSession::onSocketActivated);

    write_notifier = new QSocketNotifier(fd, QSocketNotifier::Write, this);
    write_notifier->setEnabled(false);
    connect(write_notifier, &QSocketNotifier::activated, this, &SshSession::onSocketActivated);

    checkPollFlags();
    return true;
}

bool SshSession::isSocketNotifiers() const
{
    if (!read_notifier || !write_notifier) return false;
    return (read_notifier->isEnabled() || write_notifier->isEnabled());
}

void SshSession::onSocketActivated(QSocketDescriptor sd, QSocketNotifier::Type type)
{
    Q_UNUSED(sd);
    Q_UNUSED(type);
    Q_ASSERT(read_notifier && write_notifier);

    if (!isConnected()) {
        setSocketNotifiers(false);
        setState(StateClosed);
        return;
    }
    read_notifier->setEnabled(false);
    write_notifier->setEnabled(false);

    int rc = ::ssh_event_dopoll(lib_main_event, callAgainDelay);
    TRACE_ARG("ssh_event_dopoll:" << rc)
    if (rc == SSH_ERROR) {
        setState(StateError);
        return;
    }
    if (::ssh_get_fd(lib_session.getCSession()) == SSH_INVALID_SOCKET) {
        setState(StateError);
        return;
    }
    checkPollFlags();
}

void SshSession::checkPollFlags()
{
    int flags = ::ssh_get_poll_flags(lib_session.getCSession());
    TRACE_ARG(flags);
    if (flags & (SSH_CLOSED | SSH_CLOSED_ERROR)) {
        setSocketNotifiers(false);
        setState(StateClosed);
        return;
    }
    //XXX? if (!flags) flags = SSH_READ_PENDING;
    if (read_notifier) read_notifier->setEnabled(flags & SSH_READ_PENDING);
    if (write_notifier) write_notifier->setEnabled(flags & SSH_WRITE_PENDING);
}

void SshSession::libConnect()
{
    TRACE();
    auto cs = lib_session.getCSession();
    int rc = ::ssh_connect(cs);
    if (rc == SSH_ERROR) {
        setState(StateError);
        return;
    }
    if (!isSocketNotifiers() && !setSocketNotifiers(true)) {
        setState(StateError);
        return;
    }
    setRunning(true);
    if (rc == SSH_AGAIN) {
        callLater(&SshSession::libConnect, callAgainDelay);
        return;
    }
    QString sb;
    auto cp = ::ssh_get_serverbanner(cs);
    if (cp && *cp) sb = cp;
    if (sb != hello_banner) {
        hello_banner = sb;
        emit helloBannerChanged();
    }
    setState(StateServerCheck);
}

void SshSession::libServerPublickey()
{
    TRACE();
    if (::ssh_get_server_publickey(lib_session.getCSession(), &lib_ssh_key) < 0) {
        setState(StateError);
        return;
    }
    u_char *hash = nullptr;
    size_t hlen = 0;
    int rc = ::ssh_get_publickey_hash(lib_ssh_key, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen);
    ::ssh_key_free(lib_ssh_key);
    lib_ssh_key = nullptr;
    if (rc < 0) {
        setState(StateError);
        return;
    }
    QString text;
    char *cp = ::ssh_get_hexa(hash, hlen);
    if (cp) {
        text = cp;
        ::ssh_string_free_char(cp);
    }
    ::ssh_clean_pubkey_hash(&hash);
    if (text != pubkey_hash) {
        pubkey_hash = text;
        emit pubkeyHashChanged();
    }
    libServerKnown();
}

void SshSession::libServerKnown()
{
    TRACE();
    if (session_state != StateServerCheck) {
        qWarning() << Q_FUNC_INFO << "Bad state, connect first!";
        return;
    }
    KnownHost kh = KnownHostNone;
    switch (::ssh_session_is_known_server(lib_session.getCSession())) {
    case SSH_KNOWN_HOSTS_NOT_FOUND: kh = KnownHostCreated;  break;
    case SSH_KNOWN_HOSTS_UNKNOWN:   kh = KnownHostUnknown;  break;
    case SSH_KNOWN_HOSTS_OK:        kh = KnownHostOk;       break;
    case SSH_KNOWN_HOSTS_CHANGED:   kh = KnownHostUpdated;  break;
    case SSH_KNOWN_HOSTS_OTHER:     kh = KnownHostUpgraded; break;
    default:
        setState(StateError);
        return;
    }
    if (kh != known_host) {
        known_host = kh;
        emit knownHostChanged();
    }
    if (known_host != KnownHostOk) {
        setRunning(false);
        return;
    }
    setState(StateUserAuth);
}

void SshSession::writeKnownHost()
{
    TRACE();
    if (session_state != StateServerCheck) {
        qWarning() << Q_FUNC_INFO << "Bad state, connect first!";
        return;
    }
    if (::ssh_session_update_known_hosts(lib_session.getCSession()) < 0) {
        qCritical() << Q_FUNC_INFO << "Can't write to known_hosts";
        setState(StateError);
        return;
    }
    libServerKnown();
}

void SshSession::libUserAuthNone()
{
    TRACE();
    auto cs = lib_session.getCSession();
    switch (::ssh_userauth_none(cs, nullptr)) {
    case SSH_AUTH_AGAIN:
        callLater(&SshSession::libUserAuthNone, callAgainDelay);
        return;
    case SSH_AUTH_SUCCESS:
        setState(StateEstablished);
        break;
    case SSH_AUTH_DENIED:
    case SSH_AUTH_PARTIAL:
        user_auth = ::ssh_userauth_list(cs, nullptr);
        iterateUserAuth();
        break;
    default:
        setState(StateError);
        return;
    }
}

void SshSession::iterateUserAuth()
{
    TRACE_ARG(user_auth);
    if (user_auth & SSH_AUTH_METHOD_PUBLICKEY) {
        if (!ssh_settings.privateKey().isEmpty()) {
            callLater(&SshSession::libUserAuthTryPublickey);
            return;
        }
        user_auth &= ~SSH_AUTH_METHOD_PUBLICKEY;
    }
    if (user_auth & SSH_AUTH_METHOD_INTERACTIVE) {
        if (isSignalConnected(QMetaMethod::fromSignal(&SshSession::askQuestions))) {
            callLater(&SshSession::libUserAuthKbdint);
            return;
        }
        user_auth &= ~SSH_AUTH_METHOD_INTERACTIVE;
    }
    if (user_auth & SSH_AUTH_METHOD_PASSWORD) {
        if (!ssh_settings.password().isEmpty()) {
            callLater(&SshSession::libUserAuthPassword);
            return;
        }
        if (isSignalConnected(QMetaMethod::fromSignal(&SshSession::askQuestions))) {
            setRunning(false);
            emit askQuestions(QString(), QStringList(QString("Password for %1").arg(ssh_settings.user())));
            return;
        }
        user_auth &= ~SSH_AUTH_METHOD_PASSWORD;
    }
    setState(StateDenied);
}

void SshSession::libUserAuthTryPublickey()
{
    TRACE();
    if (!lib_ssh_key) {
        if (::ssh_pki_import_pubkey_file(qPrintable(ssh_settings.privateKey() + ".pub"),
                                         &lib_ssh_key) != SSH_OK) {
            setState(StateError);
            return;
        }
        if (!lib_ssh_key) {
            setState(StateError);
            return;
        }
    }
    switch (::ssh_userauth_try_publickey(lib_session.getCSession(), nullptr, lib_ssh_key)) {
    case SSH_AUTH_AGAIN:
        callLater(&SshSession::libUserAuthTryPublickey, callAgainDelay);
        return;
    case SSH_AUTH_SUCCESS:
        callLater(&SshSession::libUserAuthPublickey);
        break;
    case SSH_AUTH_DENIED:
    case SSH_AUTH_PARTIAL:
        user_auth &= ~SSH_AUTH_METHOD_PUBLICKEY;
        iterateUserAuth();
        break;
    default:
        setState(StateError);
        break;
    }
    ::ssh_key_free(lib_ssh_key);
    lib_ssh_key = nullptr;
}

void SshSession::libUserAuthPublickey()
{
    TRACE();
    if (!lib_ssh_key) {
        if (::ssh_pki_import_privkey_file(qPrintable(ssh_settings.privateKey()),
                                          nullptr, nullptr, nullptr, &lib_ssh_key) != SSH_OK) {
            setState(StateError);
            return;
        }
        if (!lib_ssh_key) {
            setState(StateError);
            return;
        }
    }
    switch (::ssh_userauth_publickey(lib_session.getCSession(), nullptr, lib_ssh_key)) {
    case SSH_AUTH_AGAIN:
        callLater(&SshSession::libUserAuthPublickey, callAgainDelay);
        return;
    case SSH_AUTH_SUCCESS:
        setState(StateEstablished);
        break;
    case SSH_AUTH_DENIED:
    case SSH_AUTH_PARTIAL:
        user_auth &= ~SSH_AUTH_METHOD_PUBLICKEY;
        iterateUserAuth();
        break;
    default:
        setState(StateError);
        break;
    }
    ::ssh_key_free(lib_ssh_key);
    lib_ssh_key = nullptr;
}

void SshSession::libUserAuthKbdint()
{
    TRACE();
    auto cs = lib_session.getCSession();
    switch (::ssh_userauth_kbdint(cs, nullptr, nullptr)) {
    case SSH_AUTH_INFO:
        break;
    case SSH_AUTH_AGAIN:
        callLater(&SshSession::libUserAuthKbdint, callAgainDelay);
        return;
    case SSH_AUTH_SUCCESS:
        setState(StateEstablished);
        return;
    case SSH_AUTH_DENIED:
    case SSH_AUTH_PARTIAL:
        user_auth &= ~SSH_AUTH_METHOD_INTERACTIVE;
        iterateUserAuth();
        return;
    default:
        setState(StateError);
        return;
    }
    QString info;
    QStringList prompts;
    const char *cp = ::ssh_userauth_kbdint_getname(cs);
    if (cp && *cp) {
        info += cp;
        info += '\n';
    }
    cp = ::ssh_userauth_kbdint_getinstruction(cs);
    if (cp && *cp) {
        info += cp;
        info += '\n';
    }
    int num = ::ssh_userauth_kbdint_getnprompts(cs);
    for (int i = 0; i < num; i++) {
        cp = ::ssh_userauth_kbdint_getprompt(cs, i, nullptr);
        if (cp && *cp) prompts.append(cp);
    }
    if (prompts.isEmpty()) {
        callLater(&SshSession::libUserAuthKbdint);
        return;
    }
    setRunning(false);
    emit askQuestions(info, prompts);
}

void SshSession::giveAnswers(const QStringList &answers)
{
    TRACE_ARG(answers);
    if (!isConnected() || session_state != StateUserAuth) {
        qWarning() << Q_FUNC_INFO << "Bad state, connect first!";
        return;
    }
    if (!answers.isEmpty()) {
        if (user_auth & SSH_AUTH_METHOD_INTERACTIVE) {
            auto cs = lib_session.getCSession();
            int num = ::ssh_userauth_kbdint_getnprompts(cs);
            if (num != answers.size())
                qWarning() << Q_FUNC_INFO << "Number of answers mismatch questions";

            for (int i = 0; i < qMin(num, answers.size()); i++) {
                if (::ssh_userauth_kbdint_setanswer(cs, i, qPrintable(answers.at(i))) < 0) {
                    setState(StateError);
                    return;
                }
            }
            callLater(&SshSession::libUserAuthKbdint);
            setRunning(true);
            return;
        }
        if (user_auth & SSH_AUTH_METHOD_PASSWORD) {
            ssh_settings.setPassword(answers.first());
            callLater(&SshSession::libUserAuthPassword);
            setRunning(true);
            return;
        }
    }
    setState(StateDenied);
}

void SshSession::libUserAuthPassword()
{
    TRACE();
    if (ssh_settings.password().isEmpty()) {
        setState(StateDenied);
        return;
    }
    switch (::ssh_userauth_password(lib_session.getCSession(), nullptr,
                                    qPrintable(ssh_settings.password()))) {
    case SSH_AUTH_AGAIN:
        callLater(&SshSession::libUserAuthPassword, callAgainDelay);
        return;
    case SSH_AUTH_SUCCESS:
        setState(StateEstablished);
        break;
    case SSH_AUTH_DENIED:
    case SSH_AUTH_PARTIAL:
        user_auth &= ~SSH_AUTH_METHOD_PASSWORD;
        iterateUserAuth();
        break;
    default:
        setState(StateError);
        break;
    }
}

void SshSession::setOpenChannels()
{
    TRACE();
    int cnt = 0;
    for (int i = 0; i < channel_list.size(); i++) {
        if (channel_list.at(i)->isChannelOpen()) cnt++;
    }
    if (cnt != open_channels) {
        open_channels = cnt;
        emit openChannelsChanged();
    }
}

void SshSession::onChannelClosed()
{
    auto channel = reinterpret_cast<SshChannel*>(sender());
    TRACE_ARG(channel);
    if (channel && closeChannel(channel)) {
        setOpenChannels();
        if (!open_channels) disconnectFromHost();
    }
}

void SshSession::emitHostDisconnected()
{
    TRACE();
    emit hostDisconnected();
}

void SshSession::createShell(TextRender *render)
{
    TRACE_ARG(render);
    if (!render) {
        qWarning() << Q_FUNC_INFO << "A null render specified?";
        return;
    }
    if (!isReady()) {
        qWarning() << Q_FUNC_INFO << "Ssh session must be ready!";
        return;
    }
    auto shell = new SshChannelShell(lib_session, ssh_settings.termType(), ssh_settings.envVars());
    connect(shell, &SshChannelShell::channelOpened, this, &SshSession::setOpenChannels);
    connect(shell, &SshChannelShell::channelOpened, render, &TextRender::shellOpened, Qt::QueuedConnection);
    connect(shell, &SshChannelShell::textReceived, render, &TextRender::displayText);
    connect(render, &TextRender::inputText, shell, &SshChannelShell::sendText);

    connect(shell, &SshChannel::errorOccurred, this, &SshSession::setLastError);
    connect(shell, &SshChannel::channelClosed, render, &TextRender::shellClosed);
    connect(shell, &SshChannel::channelClosed, this, &SshSession::onChannelClosed, Qt::QueuedConnection);

    channel_list.append(QSharedPointer<SshChannel>(shell));
}

void SshSession::sendPublicKey()
{
    TRACE();
    if (!isEstablished()) {
        qWarning() << Q_FUNC_INFO << "Ssh session must be established!";
        return;
    }
    if (!share_key) {
        setState(StateReady);
        return;
    }
    QFileInfo info(ssh_settings.privateKey() + ".pub");
    if (!info.isFile() || !info.isReadable() || info.size() < 60) {
        setLastError(QStringLiteral("Bad public key: ") + info.filePath());
        setState(StateError);
        return;
    }
    auto ftp = new SshChannelFtp(lib_session, QLatin1String(authorizedKeys));
    connect(ftp, &SshChannelFtp::channelOpened, ftp, &SshChannelFtp::pull);
    connect(ftp, &SshChannelFtp::readyRead, this, &SshSession::onPullDone);
    connect(ftp, &SshChannel::errorOccurred, this, &SshSession::setLastError);

    channel_list.append(QSharedPointer<SshChannel>(ftp));
}

void SshSession::onPullDone()
{
    TRACE();
    auto ftp = qobject_cast<SshChannelFtp*>(sender());
    Q_ASSERT(ftp);
    QFile file(ssh_settings.privateKey() + ".pub");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ftp->close();
        setLastError(QString("Can't read %1").arg(file.fileName()));
        setState(StateError);
        return;
    }
    QByteArray public_key = file.readLine().trimmed();
    if (!public_key.startsWith("ssh-")) {
        ftp->close();
        setLastError(QString("Bad key %1").arg(file.fileName()));
        setState(StateError);
        return;
    }
    QByteArray data = ftp->readAll();
    TRACE_ARG(data.size() << data.left(100));
    if (!data.isEmpty()) {
        const auto lines = data.split('\n');
        for (int i = 0; i < lines.size(); i++) {
            if (lines.at(i).trimmed() == public_key) {
                ftp->close();
                qWarning() << Q_FUNC_INFO << "This public key is already there";
                return;
            }
        }
    }
#ifdef SFTP_USE_APPEND
    ftp->push(public_key + '\n', true);
#else // append mode is buggy, so overwrite it completely
    data += public_key;
    data += '\n';
    ftp->push(data);
#endif
    closeChannel(ftp);
    setShareKey(false);
    setState(StateReady);
}

QWeakPointer<SshChannelExec> SshSession::createExec(const QString &command)
{
    static QWeakPointer<SshChannelExec> empty;
    TRACE_ARG(command);
    if (command.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "An empty command specified?";
        return empty;
    }
    if (!isReady()) {
        qWarning() << Q_FUNC_INFO << "Ssh session must be ready!";
        return empty;
    }
    auto exec = new SshChannelExec(lib_session, command);
    connect(exec, &SshChannel::errorOccurred, this, &SshSession::setLastError);

    channel_list.append(QSharedPointer<SshChannel>(exec));
    return qWeakPointerCast<SshChannelExec>(channel_list.constLast());
}
