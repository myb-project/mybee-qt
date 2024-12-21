#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QtDebug>

#include "SystemProcess.h"

//#define TRACE_SYSTEMPROCESS
#if defined(TRACE_SYSTEMPROCESS)
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

SystemProcess::SystemProcess(QObject *parent)
    : QObject(parent)
    , run_process(nullptr)
    , now_running(false)
    , run_canceled(false)
{
    TRACE();

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &SystemProcess::cancel);
}

SystemProcess::~SystemProcess()
{
    TRACE();
}

void SystemProcess::cancel()
{
    TRACE();

    bool was_run = now_running;
    run_canceled = true;
    std_output.clear();
    std_error.clear();
    if (run_process && run_process->state() != QProcess::NotRunning) {
        was_run = true;
        run_process->kill();
        if (!QCoreApplication::closingDown())
            run_process->waitForFinished(250);
    }
    if (was_run) {
        setRunning(false);
        emit canceled();
    }
}

void SystemProcess::onErrorOccurred(QProcess::ProcessError errcode)
{
    TRACE_ARG(errcode);

    if (!run_canceled) {
        QString text;
        if (run_process) text = run_process->errorString();
        if (text.isEmpty()) {
            switch (errcode) {
            case QProcess::FailedToStart: text = "Failed to start command"; break;
            case QProcess::Crashed:       text = "Started process crashed"; break;
            case QProcess::Timedout:      text = "Started process timeout"; break;
            case QProcess::WriteError:    text = "Write to process failed"; break;
            case QProcess::ReadError:     text = "Read from process failed"; break;
            case QProcess::UnknownError: // fall through
            default: text = QString("Unknown error %1").arg(errcode);
            }
        }
        text.prepend(run_command + ": ");
        emit execError(text);
    }
    setRunning(false);
}

void SystemProcess::onFinished(int code, QProcess::ExitStatus status)
{
    TRACE_ARG(code << status);

    if (status == QProcess::NormalExit) {
        setRunning(false);
        if (!run_canceled)
            emit finished(code);
    }
}

QString SystemProcess::command() const
{
    return run_command;
}

void SystemProcess::setCommand(const QString &cmd)
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

QStringList SystemProcess::envList() const
{
    return proc_env.toStringList();
}

void SystemProcess::setEnvList(const QStringList &env_list)
{
    TRACE_ARG(env_list);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    QProcessEnvironment env;
    for (const auto &var : env_list) {
        int pos = var.indexOf('=');
        if (pos > 1) {
            QString name = var.left(pos).trimmed();
            QString value = var.mid(pos + 1).trimmed();
            if (!env.contains(name) || env.value(name) != value)
                env.insert(name, value);
        } else qWarning() << Q_FUNC_INFO << "Bad env variable" << var;
    }
    if (env != proc_env) {
        proc_env = env;
        emit envListChanged();
    }
}

//static
QStringList SystemProcess::sysEnvList()
{
    return QProcessEnvironment::systemEnvironment().toStringList();
}

QString SystemProcess::stdOutFile() const
{
    return std_out_file;
}

void SystemProcess::setStdOutFile(const QString &filename)
{
    TRACE_ARG(filename);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    if (filename != std_out_file) {
        std_out_file = filename;
        emit stdOutFileChanged();
    }
}

QString SystemProcess::stdErrFile() const
{
    return std_err_file;
}

void SystemProcess::setStdErrFile(const QString &filename)
{
    TRACE_ARG(filename);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    if (filename != std_err_file) {
        std_err_file = filename;
        emit stdErrFileChanged();
    }
}

bool SystemProcess::running() const
{
    return now_running;
}

void SystemProcess::setRunning(bool on)
{
    TRACE_ARG(on);

    if (on != now_running) {
        now_running = on;
        emit runningChanged();
    }
}

// static
QString SystemProcess::nullDevice()
{
    return QProcess::nullDevice();
}

void SystemProcess::start()
{
    TRACE_ARG(std_err_file << std_out_file);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    QStringList args = QProcess::splitCommand(run_command);
    if (args.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "An empty command specified";
        return;
    }
    if (!std_out_file.isEmpty()) {
        QFileInfo finfo(std_out_file);
        QDir dir(finfo.path());
        if (!dir.exists() && !dir.mkpath(".")) {
            qWarning() << Q_FUNC_INFO << "Can't mkpath" << std_out_file;
            return;
        }
    }
    if (!std_err_file.isEmpty()) {
        QFileInfo finfo(std_err_file);
        QDir dir(finfo.path());
        if (!dir.exists() && !dir.mkpath(".")) {
            qWarning() << Q_FUNC_INFO << "Can't mkpath" << std_err_file;
            return;
        }
    }
    if (!run_process) {
        run_process = new QProcess(this);
        connect(run_process, &QProcess::started, this, [this]() { setRunning(true); });
        connect(run_process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
                this, &SystemProcess::onFinished);
        connect(run_process, &QProcess::errorOccurred, this, &SystemProcess::onErrorOccurred);
        connect(run_process, &QProcess::readyReadStandardOutput, this, [this]() {
            if (!run_canceled) {
                QString text = QString::fromUtf8(run_process->readAllStandardOutput());
                TRACE_ARG(text);
                if (!text.isEmpty()) {
                    std_output.append(text.split('\n'));
                    emit stdOutputChanged(text);
                }
            }
        });
        connect(run_process, &QProcess::readyReadStandardError, this, [this]() {
            if (!run_canceled) {
                QString text = QString::fromUtf8(run_process->readAllStandardError());
                TRACE_ARG(text);
                if (!text.isEmpty()) {
                    std_error.append(text.split('\n'));
                    emit stdErrorChanged(text);
                }
            }
        });
    } else if (run_process->state() != QProcess::NotRunning) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    run_canceled = false;
    if (!proc_env.isEmpty()) run_process->setProcessEnvironment(proc_env);
    std_output.clear();
    std_error.clear();
    run_process->setStandardOutputFile(std_out_file);
    run_process->setStandardErrorFile(std_err_file);

    run_process->setProgram(args.takeFirst());
    if (!args.isEmpty()) run_process->setArguments(args);
#ifdef TRACE_SYSTEMPROCESS
    qDebug() << Q_FUNC_INFO << "\n\tRun" << run_process->program();
    for (const auto &arg : run_process->arguments()) {
        qDebug() << "\tArg" << arg;
    }
#endif
    run_process->start();
}

void SystemProcess::startCommand(const QString &cmd)
{
    TRACE_ARG(cmd);

    if (now_running) {
        qWarning() << Q_FUNC_INFO << "The command is currently running";
        return;
    }
    setCommand(cmd);
    start();
}

QStringList SystemProcess::stdOutput() const
{
    return std_output;
}

QStringList SystemProcess::stdError() const
{
    return std_error;
}

void SystemProcess::stdInput(const QStringList &lines)
{
    TRACE_ARG(lines);

    if (!run_process || !now_running || run_canceled) {
        qWarning() << Q_FUNC_INFO << "No running command";
        return;
    }
    for (const auto &line : lines) {
        run_process->write(line.toUtf8() + '\n');
    }
}
