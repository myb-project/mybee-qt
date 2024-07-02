#include <QUrlQuery>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QFile>

#include "SshConnection.h"
#include "SystemHelper.h"
#include "Terminal.h"

//#define TRACE_SSHCONNECTION
#ifdef TRACE_SSHCONNECTION
#include <QTime>
#include <QtDebug>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshConnection::SshConnection(QObject *parent)
    : QObject{parent}
    , user_name(SystemHelper::userName())
    , host_addr(Ssh2Settings::defaulHost)
    , port_num(Ssh2Settings::defaultPort)
    , timeout_sec(Ssh2Settings::defaultTimeout / 1000) // make seconds
    , check_server(true)
    , term_type(Terminal::defaultTermType)
    , auto_exec(true)
    , session_active(false)
    , busy_running(false)
    , exit_status(-1)
{
    TRACE_ARG(user_name << host_addr << port_num);

    last_error.clear();
}

SshConnection::~SshConnection()
{
    TRACE();
}

QString SshConnection::user() const
{
    return user_name;
}

void SshConnection::setUser(const QString &name)
{
    TRACE_ARG(name);

    QString n = !name.isEmpty() ? name : SystemHelper::userName();
    if (n != user_name) {
        user_name = n;
        emit userChanged();
    }
}

QString SshConnection::host() const
{
    return host_addr;
}

void SshConnection::setHost(const QString &addr)
{
    TRACE_ARG(addr);

    QString ha = !addr.isEmpty() ? addr : Ssh2Settings::defaulHost;
    if (ha != host_addr) {
        host_addr = ha;
        emit hostChanged();
    }
}

int SshConnection::port() const
{
    return port_num;
}

void SshConnection::setPort(int num)
{
    TRACE_ARG(num);

    int pn = (num > 0 && num < 65536) ? num : Ssh2Settings::defaultPort;
    if (pn != port_num) {
        port_num = pn;
        emit portChanged();
    }
}

int SshConnection::timeout() const
{
    return timeout_sec;
}

void SshConnection::setTimeout(int sec)
{
    TRACE_ARG(sec);

    int ts = (sec > 0 && sec <= 30) ? sec : (Ssh2Settings::defaultTimeout / 1000); // make seconds
    if (ts != timeout_sec) {
        timeout_sec = ts;
        emit timeoutChanged();
    }
}

bool SshConnection::checkServer() const
{
    return check_server;
}

void SshConnection::setCheckServer(bool enable)
{
    TRACE_ARG(enable);

    if (enable != check_server) {
        check_server = enable;
        emit checkServerChanged();
    }
}

QString SshConnection::privateKey() const
{
    return private_key;
}

void SshConnection::setPrivateKey(const QString &path)
{
    TRACE_ARG(path);

    if (path != private_key) {
        private_key = path;
        emit privateKeyChanged();
    }
}

QString SshConnection::command() const
{
    return exec_cmd;
}

void SshConnection::setCommand(const QString &cmd)
{
    TRACE_ARG(cmd);

    if (cmd != exec_cmd) {
        exec_cmd = cmd;
        emit commandChanged();
    }
}

QString SshConnection::termType() const
{
    return term_type;
}

void SshConnection::setTermType(const QString &type)
{
    TRACE_ARG(type);

    QString term = !type.isEmpty() ? type : Terminal::defaultTermType;
    if (term != term_type) {
        term_type = term;
        emit termTypeChanged();
    }
}

QStringList SshConnection::setEnv() const
{
    return set_env;
}

void SshConnection::setSetEnv(const QStringList &env)
{
    TRACE_ARG(env);

    if (env != set_env) {
        set_env = env;
        emit setEnvChanged();
    }
}

bool SshConnection::autoExec() const
{
    return auto_exec;
}

void SshConnection::setAutoExec(bool on)
{
    TRACE_ARG(on);

    if (on != auto_exec) {
        auto_exec = on;
        emit autoExecChanged();
    }
}

bool SshConnection::active() const
{
    return session_active;
}

void SshConnection::setActive(bool on)
{
    TRACE_ARG(on);

    if (on != session_active) {
        last_error.clear();
        if (!on) {
            if (ssh2_client) ssh2_client->disconnectFromHost();
            if (ssh2_process) ssh2_process->deleteLater();
            if (!conn_url.isEmpty()) {
                conn_url.clear();
                emit connectionChanged();
            }
        } else QTimer::singleShot(0, this, &SshConnection::startSession);
    }
}

void SshConnection::startSession()
{
    TRACE();

    if (host_addr.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "The host must be specified";
        return;
    }

    Ssh2Settings ssh(user_name, host_addr, port_num);
    if (!private_key.isEmpty()) ssh.setKey(private_key);
    ssh.setTimeout(timeout_sec * 1000); // make milliseconds
    if (check_server) {
        QString key_dir = SystemHelper::pathName(ssh.key());
        QFileInfo info(key_dir);
        QFile file((info.isDir() && info.isReadable() && info.isWritable()) ?
                       QDir(key_dir).filePath(knownHostsName) : SystemHelper::appConfigPath(knownHostsName));
        if (file.exists() || file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ssh.setKnownHosts(file.fileName());
            ssh.setCheckServerIdentity(true);
        } else qWarning() << Q_FUNC_INFO << "Can't create" << file.fileName();
    }
    if (!ssh.isValid()) {
        qWarning() << Q_FUNC_INFO << "The ssh settings is invalid";
        return;
    }

    QString conn("ssh://");
    conn += user_name + '@' + host_addr;
    if (port_num != Ssh2Settings::defaultPort) {
        conn += ':';
        conn += QString::number(port_num);
    }
    if (!exec_cmd.isEmpty()) {
        conn += '/';
        conn += exec_cmd;
    }
    QUrl url(conn);
    if (url != conn_url) {
        conn_url = url;
        emit connectionChanged();
    }

    if (!ssh2_client) {
        auto client = new Ssh2Client();
        connect(client, &Ssh2Client::sessionStateChanged, this, &SshConnection::onSessionStateChanged);
        connect(client, &Ssh2Client::ssh2Error, this, &SshConnection::onSsh2Error);
        connect(client, &Ssh2Client::channelsCountChanged, this, &SshConnection::channelsChanged);
        connect(client, &Ssh2Client::openChannelsCountChanged, this, &SshConnection::openChannelsChanged);
        ssh2_client.reset(client);
    }
    ssh2_client->connectToHost(ssh);
}

QUrl SshConnection::connection() const
{
    return conn_url;
}

bool SshConnection::running() const
{
    return busy_running;
}

QString SshConnection::lastError() const
{
    return checkSsh2Error(last_error) ? QString() : QString::fromStdString(last_error.message());
}

QString SshConnection::exitSignal() const
{
    return exit_signal;
}

int SshConnection::exitStatus() const
{
    return exit_status;
}

void SshConnection::runProcess(bool on)
{
    TRACE_ARG(on);

    if (!on) {
        if (busy_running) {
            busy_running = false;
            emit runningChanged();
        }
        if (ssh2_process) ssh2_process->close();
        return;
    }
    if (!session_active || busy_running) {
        qWarning() << Q_FUNC_INFO << "The session is inactive or busy";
        return;
    }

    if (!ssh2_process) {
        Q_ASSERT(!ssh2_client.isNull());

        auto proc = ssh2_client->createProcess(exec_cmd);
        connect(proc, &Ssh2Channel::ssh2Error, this, &SshConnection::onSsh2Error);
        /*connect(proc, &Ssh2Channel::channelStateChanged, this, [](Ssh2Channel::ChannelStates state) {
                qDebug() << "channelStateChanged" << state;
        });*/
        connect(proc, &Ssh2Channel::newChannelData, this, &SshConnection::onNewChannelData);
        connect(proc, &Ssh2Process::processStateChanged, ssh2_client.data(), [this](Ssh2Process::ProcessStates state) {
                TRACE_ARG(state);
                bool run = (state == Ssh2Process::Starting || state == Ssh2Process::Started || state == Ssh2Process::Finishing);
                if (run != busy_running) {
                    busy_running = run;
                    emit runningChanged();
                }
        });
        connect(proc, &Ssh2Channel::exitSignalChanged, ssh2_client.data(), [this](const QString &name) {
                TRACE_ARG(name);
                if (name != exit_signal) {
                    exit_signal = name;
                    emit exitSignalChanged(exit_signal);
                }
        });
        connect(proc, &Ssh2Channel::exitStatusChanged, ssh2_client.data(), [this](int status) {
                TRACE_ARG(status);
                if (status != exit_status) {
                    exit_status = status;
                    emit exitStatusChanged(exit_status);
                }
        });
        ssh2_process = proc;
    } else {
        ssh2_process->setCommand(exec_cmd);
    }

    if (!term_type.isEmpty())
        ssh2_process->setTermType(term_type);

    if (!set_env.isEmpty())
        ssh2_process->setSetEnv(set_env);

    QTimer::singleShot(0, ssh2_process, [this]() {
        if (!ssh2_process->open()) qWarning() << Q_FUNC_INFO << "Unable to open channel";
    });
}

bool SshConnection::start(const QUrl &url)
{
    TRACE_ARG(url);

    if (session_active || busy_running) {
        qWarning() << Q_FUNC_INFO << "The session is already active or busy";
        return false;
    }
    if (!url.isValid() || url.scheme() != "ssh") {
        qWarning() << Q_FUNC_INFO << "Invalid URL specified";
        return false;
    }

    if (!url.userName().isEmpty()) setUser(url.userName());
    if (!url.host().isEmpty()) setHost(url.host());
    if (url.port() > 0) setPort(url.port());
    if (!url.path().isEmpty()) {
        QString path = url.path();
        if (path.startsWith(QLatin1Char('/'))) path.remove(0, 1);
        if (!path.isEmpty()) setCommand(path);
    }
    if (url.hasQuery()) {
        QStringList env;
        const auto pairs = QUrlQuery(url.query()).queryItems();
        for (const auto &pair : pairs) {
            env.append(pair.first + '=' + pair.second);
        }
        setSetEnv(env);
    }

    setActive(true);
    return true;
}

bool SshConnection::execute(const QString &cmd)
{
    TRACE_ARG(cmd);

    if (!session_active || busy_running) {
        qWarning() << Q_FUNC_INFO << "The session is inactive or busy";
        return false;
    }
    if (!cmd.isEmpty()) setCommand(cmd);
    runProcess(true);
    return true;
}

void SshConnection::stdInputText(const QString &text)
{
    if (!text.isEmpty() && ssh2_process && ssh2_process->processState() == Ssh2Process::Started) {
        const QByteArray data = text.toUtf8();
        if (data.size())
            ssh2_process->writeData(data.constData(), data.size());
    }
}

void SshConnection::onSessionStateChanged(Ssh2Client::SessionStates ssh2_state)
{
    TRACE_ARG(ssh2_state);

    switch (ssh2_state) {
    case Ssh2Client::NotEstableshed:
    case Ssh2Client::FailedToEstablish:
    case Ssh2Client::Closed:
    case Ssh2Client::Aborted:
        runProcess(false);
        if (session_active) {
            session_active = false;
            emit activeChanged();
        }
        break;
    case Ssh2Client::Established:
        if (!session_active) {
            session_active = true;
            emit activeChanged();
        }
        break;
    case Ssh2Client::StartingSession:
    case Ssh2Client::GetAuthMethods:
    case Ssh2Client::Authentication:
    case Ssh2Client::Closing:
        if (!busy_running) {
            busy_running = true;
            emit runningChanged();
        }
        return;
    }

    if (busy_running) {
        busy_running = false;
        emit runningChanged();
    }

    if (ssh2_state == Ssh2Client::Established && auto_exec)
        runProcess(true);
}

void SshConnection::onSsh2Error(std::error_code ssh2_error)
{
    TRACE_ARG(ssh2_error.message());

    if (ssh2_error != last_error) {
        last_error = ssh2_error;
        emit lastErrorChanged();
    }
}

void SshConnection::onNewChannelData(QByteArray data, Ssh2Channel::ChannelStream stream_id)
{
    TRACE_ARG(stream_id << "<==" << data);

    switch (stream_id) {
    case Ssh2Process::Out:
        emit stdOutputChanged(data);
        break;
    case Ssh2Process::Err:
        emit stdErrorChanged(data);
        break;
    default:
        qWarning() << Q_FUNC_INFO << "Unexpected channel stream id" << stream_id;
        return;
    }
}
