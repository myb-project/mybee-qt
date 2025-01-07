#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QtDebug>

#include "SshProcess.h"
#include "SshSession.h"
#include "SshChannelExec.h"

//#define TRACE_SSHPROCESS
#ifdef  TRACE_SSHPROCESS
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SshProcess::SshProcess(QObject *parent)
    : QObject(parent)
    , now_running(false)
    , now_executing(false)
    , exit_status(-1)
{
    TRACE();
    qRegisterMetaType<SshSettings>("SshSettings");
}

SshProcess::~SshProcess()
{
    TRACE();
}

void SshProcess::setSettings(const SshSettings &settings)
{
    TRACE_ARG(settings);

    if (!settings.isValid()) {
        qWarning() << Q_FUNC_INFO << "The settings is invalid";
        return;
    }
    if (settings != ssh_settings) {
        cancel();
        ssh_settings = settings;
        emit settingsChanged();
    }
}

void SshProcess::cancel()
{
    TRACE();

    if (ssh_session && ssh_session->running()) {
        onChannelClosed();
        emit canceled();
    }
}

void SshProcess::setCommand(const QString &cmd)
{
    TRACE_ARG(cmd);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    if (cmd != run_command) {
        run_command = cmd;
        emit commandChanged();
    }
}

void SshProcess::setStdOutFile(const QString &filename)
{
    TRACE_ARG(filename);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    if (std_out_file.isOpen()) std_out_file.close();
    if (filename != std_out_file.fileName()) {
        std_out_file.setFileName(filename);
        emit stdOutFileChanged();
    }
}

void SshProcess::setStdErrFile(const QString &filename)
{
    TRACE_ARG(filename);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    if (std_err_file.isOpen()) std_err_file.close();
    if (filename != std_err_file.fileName()) {
        std_err_file.setFileName(filename);
        emit stdErrFileChanged();
    }
}

void SshProcess::setRunning(bool on)
{
    TRACE_ARG(on);

    if (on != now_running) {
        now_running = on;
        emit runningChanged();
    }
}

void SshProcess::setExecuting(bool on)
{
    TRACE_ARG(on);

    if (on != now_executing) {
        now_executing = on;
        emit executingChanged();
    }
}

void SshProcess::start(const QUrl &url, const QString &key)
{
    TRACE_ARG(url << key);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    if (!url.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid URL specified";
        return;
    }
    if (key.isEmpty() || !QFile::exists(key)) {
        qWarning() << Q_FUNC_INFO << "Invalid private key specified";
        return;
    }
    if (run_command.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "An empty command specified";
        return;
    }
    setSettings(SshSettings::fromUrl(url, key));

    if (!ssh_session) {
        auto ss = new SshSession(this);
        connect(ss, &SshSession::stateChanged, this, &SshProcess::onStateChanged);
        connect(ss, &SshSession::hostDisconnected, ss, &QObject::deleteLater);
        connect(ss, &SshSession::lastErrorChanged, this, &SshProcess::onLastErrorChanged);
        connect(ss, &SshSession::shareKeyChanged, this, &SshProcess::shareKeyChanged);
        connect(ss, &SshSession::hostAddressChanged, this, &SshProcess::hostAddressChanged);
        connect(ss, &SshSession::pubkeyHashChanged, this, &SshProcess::pubkeyHashChanged);
        connect(ss, &SshSession::knownHostChanged, this, &SshProcess::knownHostChanged);
        connect(ss, &SshSession::askQuestions, this, &SshProcess::askQuestions);
        ssh_session = ss;
    } else if (ssh_session->channels()) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    ssh_session->setSettings(ssh_settings);

    if (std_out_file.isOpen()) std_out_file.close();
    if (std_err_file.isOpen()) std_err_file.close();
    std_output.clear();
    std_error.clear();

    QUrl cmd(ssh_settings.toString());
    cmd.setQuery(run_command);
    if (cmd != ssh_url) {
        ssh_url = cmd;
        emit sshUrlChanged();
    }
    exit_status = -1;
    ssh_session->connectToHost();
    setRunning(true);
}

void SshProcess::onStateChanged()
{
    TRACE_ARG(ssh_session->state());
    if (ssh_session->state() != SshSession::StateReady) return;

    ssh_exec = ssh_session->createExec(run_command);
    if (!ssh_exec) {
        qWarning() << Q_FUNC_INFO << "createExec() return null pointer";
        return;
    }
    auto exec = ssh_exec.toStrongRef().data();
    connect(exec, &SshChannelExec::channelOpened, this, &SshProcess::onChannelOpened);
    connect(exec, &SshChannelExec::exitStatusChanged, this, [this](int code) { exit_status = code; });
    connect(exec, &SshChannel::readyRead, this, [this,exec]() {
        if (!exec->currentReadChannel())
             readStdoutData(exec->readAll());
        else readStderrData(exec->readAll());
    });
    connect(exec, &SshChannel::channelClosed, this, &SshProcess::onChannelClosed, Qt::QueuedConnection);
}

void SshProcess::onChannelOpened()
{
    TRACE();

    QString path = std_out_file.fileName();
    if (!path.isEmpty()) {
        QFileInfo finfo(path);
        QDir dir(finfo.path());
        if (dir.exists() || dir.mkpath(".")) {
            if (std_out_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                std_out_file.resize(0);
            } else qWarning() << Q_FUNC_INFO << "Can't write" << path;
        } else qWarning() << Q_FUNC_INFO << "Can't mkpath" << path;
    }
    path = std_err_file.fileName();
    if (!path.isEmpty()) {
        QFileInfo finfo(path);
        QDir dir(finfo.path());
        if (dir.exists() || dir.mkpath(".")) {
            if (std_err_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                std_out_file.resize(0);
            } else qWarning() << Q_FUNC_INFO << "Can't write" << path;
        } else qWarning() << Q_FUNC_INFO << "Can't mkpath" << path;
    }
    setExecuting(true);
}

void SshProcess::onChannelClosed()
{
    TRACE();

    if (std_out_file.isOpen()) std_out_file.close();
    if (std_err_file.isOpen()) std_err_file.close();
    if (ssh_session) {
        ssh_session->disconnect(this);
        ssh_session->deleteLater();
    }
    if (now_executing) setExecuting(false);
    if (now_running) setRunning(false);
    emit finished(exit_status);
}

void SshProcess::onLastErrorChanged(const QString &text)
{
    TRACE();

    onChannelClosed();
    emit execError(ssh_url.toString() + "\n\n" + (!text.isEmpty() ? text : QStringLiteral("Unknown error")));
}

void SshProcess::readStdoutData(const QByteArray &data)
{
    TRACE_ARG(data.size());
    if (data.isEmpty()) return;

    if (std_out_file.isOpen() && std_out_file.write(data) == data.size())
        return;
    QString text = QString::fromUtf8(data);
    std_output.append(text.split('\n'));
    emit stdOutputChanged(text);
}

void SshProcess::readStderrData(const QByteArray &data)
{
    TRACE_ARG(data.size());
    if (data.isEmpty()) return;

    if (std_err_file.isOpen() && std_err_file.write(data) == data.size())
        return;
    QString text = QString::fromUtf8(data);
    std_error.append(text.split('\n'));
    emit stdOutputChanged(text);
}

void SshProcess::stdInput(const QStringList &lines)
{
    TRACE_ARG(lines);

    if (!now_executing) {
        qWarning() << Q_FUNC_INFO << "No running command";
        return;
    }
    for (const auto &line : lines) {
        if (!ssh_exec) break;
        ssh_exec.toStrongRef()->write(line.toUtf8() + '\n');
    }
}

// The following properties are inherited from the SshSession class
void SshProcess::setShareKey(bool enable)
{
    TRACE_ARG(enable);
    if (ssh_session) ssh_session->setShareKey(enable);
}

bool SshProcess::shareKey() const
{
    return (ssh_session ? ssh_session->shareKey() : false);
}

QString SshProcess::hostAddress() const
{
    return (ssh_session ? ssh_session->hostAddress() : QString());
}

QString SshProcess::pubkeyHash() const
{
    return (ssh_session ? ssh_session->pubkeyHash() : QString());
}

int SshProcess::knownHost() const
{
    return (ssh_session ? ssh_session->knownHost() : SshSession::KnownHostNone);
}

void SshProcess::writeKnownHost()
{
    TRACE();
    if (ssh_session) ssh_session->writeKnownHost();
}

void SshProcess::giveAnswers(const QStringList &answers)
{
    TRACE_ARG(answers);
    if (ssh_session) ssh_session->giveAnswers(answers);
}
/*
// static
SshSettings SshProcess::sshSettings(const QString &user, const QString &host, quint16 port, const QString &key)
{
    return SshSettings(user, host, port, key);
}
*/
