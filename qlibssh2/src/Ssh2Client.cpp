/*
MIT License

Copyright (c) 2023 Vladimir Vorobyev <b800xy@gmail.com>
Copyright (c) 2020 Mikhail Milovidov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "Ssh2Client.h"
#include "Ssh2Channel.h"
#include "Ssh2LocalPortForwarding.h"
#include "Ssh2Process.h"
#include "Ssh2Scp.h"

#include <libssh2.h>

#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QCoreApplication>

//#define TRACE_SSH2CLIENT
#ifdef TRACE_SSH2CLIENT
#include <QTime>
#include <QtDebug>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

std::atomic<size_t> ssh2_initializations_count(0);

static void freeSsh2()
{
    if (ssh2_initializations_count == 1) {
        TRACE();

        libssh2_exit();
    }
    if (ssh2_initializations_count > 0)
        ssh2_initializations_count--;
}

static void initializeSsh2()
{
    if (ssh2_initializations_count == 0) {
        TRACE();

        libssh2_init(0);
        auto app = QCoreApplication::instance();
        QObject::connect(app, &QCoreApplication::aboutToQuit, app, []() { freeSsh2(); });
    }
    ssh2_initializations_count++;
}

static ssize_t libssh_recv(int socket, void* buffer, size_t length, int flags, void** abstract)
{
    Q_UNUSED(socket);
    Q_UNUSED(flags);
    QTcpSocket* const tcp_socket = reinterpret_cast<QTcpSocket*>(*abstract);
    char* const data = reinterpret_cast<char*>(buffer);
    ssize_t result = tcp_socket->read(data, length);
    if (result == 0)
        result = -EAGAIN;
    return result;
}

static ssize_t libssh_send(int socket, const void* buffer, size_t length, int flags, void** abstract)
{
    Q_UNUSED(socket);
    Q_UNUSED(flags);
    QTcpSocket* const tcp_socket = reinterpret_cast<QTcpSocket*>(*abstract);
    const char* const data = reinterpret_cast<const char*>(buffer);
    ssize_t result = tcp_socket->write(data, length);
    if (result == 0)
        result = -EAGAIN;
    return result;
}

Ssh2Client::Ssh2Client(QObject *parent)
    : QTcpSocket(parent)
    , ssh2_state_(SessionStates::NotEstableshed)
    , ssh2_auth_method_(Ssh2AuthMethods::NoAuth)
    , last_error_(ssh2_success)
    , ssh2_session_(nullptr)
    , known_hosts_(nullptr)
{
    TRACE();

    connect(this, &QTcpSocket::connected, this, &Ssh2Client::onTcpConnected);
    connect(this, &QTcpSocket::disconnected, this, &Ssh2Client::onTcpDisconnected);
    connect(this, &QTcpSocket::readyRead, this, &Ssh2Client::onReadyRead);
    connect(this, &QTcpSocket::stateChanged, this, &Ssh2Client::onSocketStateChanged);

    initializeSsh2();
}

Ssh2Client::Ssh2Client(const Ssh2Settings &settings, QObject* parent)
    : Ssh2Client(parent)
{
    ssh2_settings_ = settings;

    TRACE_ARG(ssh2_settings_);
}

Ssh2Client::~Ssh2Client()
{
    TRACE();

    closeSession();
    if (state() != UnconnectedState)
        waitForDisconnected(2500);
}

bool Ssh2Client::connectToHost(const Ssh2Settings &settings)
{
    TRACE_ARG(settings);

    if (!settings.isValid()) return false;

    ssh2_settings_ = settings;

    QTcpSocket::connectToHost(ssh2_settings_.host(), ssh2_settings_.port());
    return true;
}

void Ssh2Client::connectToHost(const QString& hostName, qint16 port)
{
    auto splitted = hostName.split(QLatin1Char{'@'});
    if (splitted.count() == 2) {
        ssh2_settings_.setUser(splitted.at(0));
        ssh2_settings_.setHost(splitted.at(1));
    } else ssh2_settings_.setHost(hostName);
    ssh2_settings_.setPort(port);

    QTcpSocket::connectToHost(hostName, port);
}

void Ssh2Client::connectToHost(const QString& userName, const QString& hostName, quint16 port)
{
    ssh2_settings_.setUser(userName);
    ssh2_settings_.setHost(hostName);
    ssh2_settings_.setPort(port);

    QTcpSocket::connectToHost(hostName, port);
}

Ssh2Client::SessionStates Ssh2Client::sessionState() const
{
    return ssh2_state_;
}

void Ssh2Client::closeChannels()
{
    TRACE();

    for (Ssh2Channel* ssh2_channel : getChannels()) {
        ssh2_channel->close();
    }
}

void Ssh2Client::closeSession()
{
    TRACE();

    if (ssh2_state_ != FailedToEstablish)
        setSsh2SessionState(Closed);
}

void Ssh2Client::checkConnection()
{
    TRACE();

    if (state() != QAbstractSocket::ConnectedState) {
        setSsh2SessionState(FailedToEstablish, Ssh2Error::ConnectionTimeoutError);
    }
}

void Ssh2Client::disconnectFromHost()
{
    TRACE();

    if (state() == QAbstractSocket::UnconnectedState)
        return;
    switch (ssh2_state_) {
    case Established: {
        if (openChannelsCount() > 0) {
            setSsh2SessionState(Closing);
        } else {
            setSsh2SessionState(Closed);
        }
    } break;
    case Closing:
        destroySsh2Objects();
        break;
    default:;
    }

    QAbstractSocket::disconnectFromHost();
}

void Ssh2Client::onTcpConnected()
{
    TRACE();

    std::error_code error_code = createSsh2Objects();
    if (!error_code)
        error_code = startSshSession();
    if (!checkSsh2Error(error_code))
        setSsh2SessionState(FailedToEstablish, error_code);
}

void Ssh2Client::onTcpDisconnected()
{
    TRACE();

    if (ssh2_state_ != Closed)
        setSsh2SessionState(Aborted, Ssh2Error::TcpConnectionRefused);
}

std::error_code Ssh2Client::startSshSession()
{
    std::error_code error_code = ssh2_success;
    const qintptr socket_descriptor = socketDescriptor();
    if (socket_descriptor == -1) {
        TRACE_ARG("socketDescriptor -1");
        setSsh2SessionState(SessionStates::FailedToEstablish, Ssh2Error::SessionStartupError);
        return error_code;
    }

    int ssh2_method_result = libssh2_session_startup(ssh2_session_,
                                                     socket_descriptor);

    switch (ssh2_method_result) {
    case LIBSSH2_ERROR_EAGAIN:
        TRACE_ARG("LIBSSH2_ERROR_EAGAIN");
        setSsh2SessionState(SessionStates::StartingSession);
        error_code = Ssh2Error::TryAgain;
        break;
    case 0:
        TRACE();
        error_code = checkKnownHosts();
        if (!error_code)
            error_code = getAvailableAuthMethods();
        break;
    default:
        TRACE_ARG("ssh2_method_result" << ssh2_method_result);
        error_code = Ssh2Error::SessionStartupError;
    }

    if (!checkSsh2Error(error_code)) {
        debugSsh2Error(ssh2_method_result);
    }

    return error_code;
}

void Ssh2Client::onReadyRead()
{
    TRACE();

    std::error_code error_code = ssh2_success;
    switch (ssh2_state_) {
    case SessionStates::StartingSession:
        error_code = startSshSession();
        break;
    case SessionStates::GetAuthMethods:
        error_code = getAvailableAuthMethods();
        break;
    case SessionStates::Authentication:
        error_code = authenticate();
        break;
    case SessionStates::Established:
    case SessionStates::Closing:
        for (Ssh2Channel* ssh2_channel : getChannels()) {
            ssh2_channel->checkIncomingData();
        }
        break;
    default:;
    }

    if (ssh2_state_ != SessionStates::Established && !checkSsh2Error(error_code)) {
        setSsh2SessionState(SessionStates::FailedToEstablish, error_code);
    }
}

void Ssh2Client::onChannelStateChanged(int state)
{
    TRACE_ARG(state);

    switch (static_cast<Ssh2Channel::ChannelStates>(state)) {
    case Ssh2Channel::Closed:
    case Ssh2Channel::Opened:
    case Ssh2Channel::FailedToOpen:
        emit openChannelsCountChanged(openChannelsCount());
        break;
    default:;
    }
    if (ssh2_state_ == Closing && openChannelsCount() == 0)
        setSsh2SessionState(Closed);
}

void Ssh2Client::onSocketStateChanged(const QAbstractSocket::SocketState& state)
{
    TRACE_ARG(state);

    switch (state) {
    case QAbstractSocket::UnconnectedState:
        if (ssh2_state_ != NotEstableshed) {
            setSsh2SessionState(FailedToEstablish, Ssh2Error::TcpConnectionError);
        }
        break;
    case QAbstractSocket::ConnectingState:
        QTimer::singleShot(ssh2_settings_.timeout(), this, &Ssh2Client::checkConnection);
        break;
    default:;
    }
}

void Ssh2Client::addChannel(Ssh2Channel* channel)
{
    TRACE_ARG(channel << channel->channelState());

    disconnect(channel);
    connect(channel, &Ssh2Channel::channelStateChanged, this, &Ssh2Client::onChannelStateChanged);
    connect(channel, &Ssh2Channel::destroyed, this, [this](QObject*) {
        emit channelsCountChanged(channelsCount());
    });
    emit channelsCountChanged(channelsCount());
}

QList<Ssh2Channel*> Ssh2Client::getChannels() const
{
    TRACE();

    return findChildren<Ssh2Channel*>();
}

void Ssh2Client::destroySsh2Objects()
{
    TRACE();

    for (Ssh2Channel* channel : getChannels()) {
        channel->deleteLater();
    }

    if (known_hosts_) {
        libssh2_knownhost_free(known_hosts_);
        known_hosts_ = nullptr;
    }
    if (ssh2_session_) {
        libssh2_session_disconnect(ssh2_session_, "disconnect");
        libssh2_session_free(ssh2_session_);
        ssh2_session_ = nullptr;
    }
    ssh2_available_auth_methods_.clear();
    ssh2_auth_method_ = Ssh2AuthMethods::NoAuth;

    if (state() == QTcpSocket::ConnectedState)
        QTcpSocket::disconnectFromHost();
    else QTcpSocket::abort();
}

std::error_code Ssh2Client::createSsh2Objects()
{
    TRACE();

    if (ssh2_session_)
        return ssh2_success;

    ssh2_session_ = libssh2_session_init_ex(nullptr, nullptr, nullptr, reinterpret_cast<void*>(this));
    if (ssh2_session_ == nullptr)
        return Ssh2Error::UnexpectedError;

    libssh2_session_callback_set(ssh2_session_, LIBSSH2_CALLBACK_RECV, reinterpret_cast<void*>(&libssh_recv));
    libssh2_session_callback_set(ssh2_session_, LIBSSH2_CALLBACK_SEND, reinterpret_cast<void*>(&libssh_send));

    if (ssh2_settings_.checkServerIdentity()) {
        known_hosts_ = libssh2_knownhost_init(ssh2_session_);
        if (known_hosts_ == nullptr)
            return Ssh2Error::UnexpectedError;

        if (ssh2_settings_.isKeyAuth()) {
            const int ssh2_method_result = libssh2_knownhost_readfile(
                known_hosts_,
                qPrintable(ssh2_settings_.knownHosts()),
                LIBSSH2_KNOWNHOST_FILE_OPENSSH);
            if (ssh2_method_result < 0)
                return Ssh2Error::ErrorReadKnownHosts;
        }
    }

    libssh2_session_set_blocking(ssh2_session_, 0);

    return ssh2_success;
}

std::error_code Ssh2Client::checkKnownHosts() const
{
    TRACE();

    if (ssh2_settings_.isPasswordAuth()) {
        TRACE_ARG("password auth");
        return ssh2_success;
    }
    if (!ssh2_settings_.checkServerIdentity())
        return ssh2_success;

    if (!known_hosts_)
        return Ssh2Error::UnexpectedError;

    size_t length;
    int type;
    const char* fingerprint = libssh2_session_hostkey(ssh2_session_, &length, &type);
    if (fingerprint == nullptr) {
        return Ssh2Error::HostKeyInvalidError;
    }

    std::error_code result = ssh2_success;
    if (fingerprint) {
        TRACE_ARG("host" << ssh2_settings_.host() << "key" << fingerprint << "length" << length);

        struct libssh2_knownhost* host = nullptr;
        const int check = libssh2_knownhost_check(known_hosts_,
                                                  qPrintable(ssh2_settings_.host()),
                                                  fingerprint,
                                                  length,
                                                  LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW,
                                                  &host);

        switch (check) {
        case LIBSSH2_KNOWNHOST_CHECK_MATCH:
            TRACE_ARG("LIBSSH2_KNOWNHOST_CHECK_MATCH");
            result = ssh2_success;
            break;
        case LIBSSH2_KNOWNHOST_CHECK_FAILURE:
            TRACE_ARG("LIBSSH2_KNOWNHOST_CHECK_FAILURE");
            result = Ssh2Error::HostKeyInvalidError;
            break;
        case LIBSSH2_KNOWNHOST_CHECK_MISMATCH:
            TRACE_ARG("LIBSSH2_KNOWNHOST_CHECK_MISMATCH");
            result = Ssh2Error::HostKeyMismatchError;
            break;
        case LIBSSH2_KNOWNHOST_CHECK_NOTFOUND:
            TRACE_ARG("LIBSSH2_KNOWNHOST_CHECK_NOTFOUND");
            if (!ssh2_settings_.autoAppendToKnownHosts().isEmpty()) {
                result = appendToKnownHosts(ssh2_settings_.autoAppendToKnownHosts());
            } else {
                result = Ssh2Error::HostKeyUnknownError;
            }
            break;
        }
    }
    return result;
}

std::error_code Ssh2Client::getAvailableAuthMethods()
{
    TRACE();

    std::error_code result = ssh2_success;
    int ssh2_method_result = 0;
    const char* available_list = libssh2_userauth_list(ssh2_session_,
                                                       qPrintable(ssh2_settings_.user()),
                                                       ssh2_settings_.user().length());

    if (available_list == nullptr) {
        ssh2_method_result = libssh2_session_last_error(ssh2_session_, nullptr, nullptr, 0);
        if (ssh2_method_result == LIBSSH2_ERROR_EAGAIN) {
            setSsh2SessionState(SessionStates::GetAuthMethods);
            return Ssh2Error::TryAgain;
        }
    }

    if (available_list != nullptr) {
        foreach (QByteArray method, QByteArray(available_list).split(',')) {
            if (method == "publickey") {
                ssh2_available_auth_methods_ << Ssh2AuthMethods::PublicKeyAuthentication;
            } else if (method == "password") {
                ssh2_available_auth_methods_ << Ssh2AuthMethods::PasswordAuthentication;
            }
        }
        ssh2_auth_method_ = getAuthenticationMethod(ssh2_available_auth_methods_);
        result = authenticate();
    } else if (ssh2_method_result != 0) {
        result = Ssh2Error::UnexpectedError;
        debugSsh2Error(ssh2_method_result);
    }
    return result;
}

Ssh2Client::Ssh2AuthMethods Ssh2Client::getAuthenticationMethod(const QList<Ssh2AuthMethods>& available_auth_methods) const
{
    TRACE_ARG(available_auth_methods);

    Ssh2AuthMethods result = Ssh2AuthMethods::NoAuth;
    if (available_auth_methods.isEmpty())
        result = Ssh2AuthMethods::NoAuth;
    else if (available_auth_methods.contains(Ssh2AuthMethods::PasswordAuthentication) && ssh2_settings_.isPasswordAuth()) {
        result = Ssh2AuthMethods::PasswordAuthentication;
    } else if (available_auth_methods.contains(Ssh2AuthMethods::PublicKeyAuthentication) && ssh2_settings_.isKeyAuth()) {
        result = Ssh2AuthMethods::PublicKeyAuthentication;
    }

    return result;
}

std::error_code Ssh2Client::authenticate()
{
    TRACE();

    std::error_code result = ssh2_success;
    int ssh2_method_result = 0;
    switch (ssh2_auth_method_) {
    case Ssh2AuthMethods::NoAuth:
        ssh2_method_result = libssh2_userauth_authenticated(ssh2_session_);
        break;
    case Ssh2AuthMethods::PublicKeyAuthentication:
        ssh2_method_result = libssh2_userauth_publickey_fromfile(
            ssh2_session_,
            qPrintable(ssh2_settings_.user()),
            nullptr,
            qPrintable(ssh2_settings_.key()),
            qPrintable(ssh2_settings_.keyPhrase()));
        break;
    case Ssh2AuthMethods::PasswordAuthentication:
        ssh2_method_result = libssh2_userauth_password(
            ssh2_session_,
            qPrintable(ssh2_settings_.user()),
            qPrintable(ssh2_settings_.passPhrase()));
        break;
    }
    switch (ssh2_method_result) {
    case LIBSSH2_ERROR_EAGAIN:
        setSsh2SessionState(SessionStates::Authentication);
        result = Ssh2Error::TryAgain;
        break;
    case 0:
        result = ssh2_success;
        setSsh2SessionState(SessionStates::Established);
        break;
    default: {
        debugSsh2Error(ssh2_method_result);
        result = Ssh2Error::AuthenticationError;
    }
    }

    return result;
}

std::error_code Ssh2Client::setLastError(const std::error_code& error_code)
{
    TRACE_ARG(&error_code);

    if (last_error_ != error_code && error_code != Ssh2Error::TryAgain) {
        last_error_ = error_code;
        emit ssh2Error(last_error_);
    }
    return error_code;
}

LIBSSH2_SESSION* Ssh2Client::ssh2Session() const
{
    return ssh2_session_;
}

QPointer<Ssh2Process> Ssh2Client::createProcess(const QString& command)
{
    TRACE_ARG(command);

    Ssh2Process *ssh2_process = new Ssh2Process(command, this);
    addChannel(ssh2_process);
    return ssh2_process;
}

QPointer<Ssh2Scp> Ssh2Client::scpSend(const QString& localFilePath, const QString& destinationPath)
{
    TRACE_ARG(localFilePath << destinationPath);

    auto* ssh2_scp = new Ssh2Scp(localFilePath, destinationPath, Ssh2Scp::Send, this);
    addChannel(ssh2_scp);
    return ssh2_scp;
}

QPointer<Ssh2Scp> Ssh2Client::scpReceive(const QString& remoteFilePath, const QString& destinationPath)
{
    TRACE_ARG(remoteFilePath << destinationPath);

    auto* ssh2_scp = new Ssh2Scp(remoteFilePath, destinationPath, Ssh2Scp::Receive, this);
    addChannel(ssh2_scp);
    return ssh2_scp;
}

QPointer<Ssh2LocalPortForwarding> Ssh2Client::localPortForwarding(quint16 localListenPort, const QHostAddress& remoteHost, quint16 remoteListenPort)
{
    TRACE_ARG(localListenPort << remoteHost << remoteListenPort);

    return localPortForwarding(QHostAddress::LocalHost, localListenPort,
                               remoteHost, remoteListenPort);
}

QPointer<Ssh2LocalPortForwarding> Ssh2Client::localPortForwarding(const QHostAddress &localListenHost, quint16 localListenPort,
                                                                  const QHostAddress& remoteHost, quint16 remoteListenPort)
{
    TRACE_ARG(localListenHost << localListenPort << remoteHost << remoteListenPort);

    auto* ssh2_local_port_forwarding = new Ssh2LocalPortForwarding(localListenHost, localListenPort,
                                                                   remoteHost, remoteListenPort, this);
    addChannel(ssh2_local_port_forwarding);
    return ssh2_local_port_forwarding;
}

int Ssh2Client::channelsCount() const
{
    return getChannels().size();
}

int Ssh2Client::openChannelsCount() const
{
    TRACE();

    int result = 0;
    for (Ssh2Channel* channel : getChannels()) {
        if (channel->isOpen())
            result++;
    }
    return result;
}

std::error_code Ssh2Client::appendToKnownHosts(const QString &comment) const
{
    TRACE_ARG(comment);

    if (!known_hosts_)
        return Ssh2Error::UnexpectedError;

    size_t hostKeyLength{0};
    int hostKeyType{0};
    auto* hostKey = libssh2_session_hostkey(ssh2_session_, &hostKeyLength, &hostKeyType);
    if (!hostKey)
        return Ssh2Error::HostKeyInvalidError;

    int knownHostKeyType{0};
    switch (hostKeyType) {
    case LIBSSH2_HOSTKEY_TYPE_UNKNOWN:
        return Ssh2Error::HostKeyUnknownError;
    case LIBSSH2_HOSTKEY_TYPE_RSA:
        knownHostKeyType = LIBSSH2_KNOWNHOST_KEY_RSA1;
        break;
    case LIBSSH2_HOSTKEY_TYPE_DSS:
        knownHostKeyType = LIBSSH2_KNOWNHOST_KEY_SSHDSS;
        break;
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_256:
        knownHostKeyType = LIBSSH2_KNOWNHOST_KEY_ECDSA_256;
        break;
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_384:
        knownHostKeyType = LIBSSH2_KNOWNHOST_KEY_ECDSA_384;
        break;
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_521:
        knownHostKeyType = LIBSSH2_KNOWNHOST_KEY_ECDSA_521;
        break;
    case LIBSSH2_HOSTKEY_TYPE_ED25519:
        knownHostKeyType = LIBSSH2_KNOWNHOST_KEY_ED25519;
        break;
    }

    if (libssh2_knownhost_addc(known_hosts_,
                               qPrintable(ssh2_settings_.host()),
                               nullptr,
                               hostKey, hostKeyLength,
                               qPrintable(comment), comment.length(),
                               LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW | knownHostKeyType,
                               nullptr)) {
        return Ssh2Error::HostKeyAppendError;
    }

    if (libssh2_knownhost_writefile(known_hosts_,
                                    qPrintable(ssh2_settings_.knownHosts()),
                                    LIBSSH2_KNOWNHOST_FILE_OPENSSH)) {
        return Ssh2Error::ErrorWriteKnownHosts;
    }

    return ssh2_success;
}

void Ssh2Client::setSsh2SessionState(const Ssh2Client::SessionStates& new_state)
{
    TRACE_ARG(new_state);

    if (ssh2_state_ != new_state) {
        switch (new_state) {
        case Closing:
            closeChannels();
            break;
        case FailedToEstablish:
            [[fallthrough]];
        case Closed:
            [[fallthrough]];
        case Aborted:
            destroySsh2Objects();
            break;
        default:;
        }
        ssh2_state_ = new_state;
        emit sessionStateChanged(new_state);
    }
}

const std::error_code& Ssh2Client::setSsh2SessionState(const SessionStates& new_state,
                                                       const std::error_code& error_code)
{
    TRACE_ARG(new_state << &error_code);

    setLastError(error_code);
    setSsh2SessionState(new_state);
    return error_code;
}
