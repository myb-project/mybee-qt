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
#include "Ssh2Process.h"
#include "Ssh2Client.h"

#include <libssh2.h>

//#define TRACE_SSH2PROCESS
#ifdef TRACE_SSH2PROCESS
#include <QTime>
#include <QtDebug>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

Ssh2Process::Ssh2Process(const QString& cmd, Ssh2Client* ssh2_client)
    : Ssh2Channel(ssh2_client)
    , command_(cmd)
    , request_setenv(0)
    , request_terminal(0)
    , terminal_type("xterm-256color")
{
    TRACE_ARG(command_);

    connect(this, &Ssh2Channel::channelStateChanged, this, &Ssh2Process::onSsh2ChannelStateChanged);
}

Ssh2Process::~Ssh2Process()
{
    TRACE();
}

Ssh2Process::ProcessStates Ssh2Process::processState() const
{
    return ssh2_process_state_;
}

void Ssh2Process::checkIncomingData()
{
    TRACE_ARG(processState());

    Ssh2Channel::checkIncomingData();
    if (processState() == Starting)
        setLastError(execSsh2Process());
}

void Ssh2Process::setSsh2ProcessState(ProcessStates ssh2_process_state)
{
    TRACE_ARG(ssh2_process_state);

    if (ssh2_process_state_ == ssh2_process_state)
        return;

    ssh2_process_state_ = ssh2_process_state;
    emit processStateChanged(ssh2_process_state_);
}

void Ssh2Process::onSsh2ChannelStateChanged(const ChannelStates& state)
{
    TRACE_ARG(state);

    std::error_code error_code = ssh2_success;
    switch (state) {
    case ChannelStates::Opened:
        request_setenv = 0;
        request_terminal = command_.isEmpty() ? 1 : 0;
        error_code = execSsh2Process();
        break;
    case ChannelStates::Closing:
        if (ssh2_process_state_ != FailedToStart)
            setSsh2ProcessState(Finishing);
        break;
    case ChannelStates::Closed:
        if (ssh2_process_state_ != FailedToStart)
            setSsh2ProcessState(Finished);
        break;
    case ChannelStates::FailedToOpen:
        setSsh2ProcessState(FailedToStart);
        error_code = Ssh2Error::ProcessFailedToStart;
        break;
    default:;
    }
    setLastError(error_code);
}

std::error_code Ssh2Process::execSsh2Process()
{
    TRACE_ARG(ssh2_process_state_ << command_);

    int ssh2_method_result = 0;

    while (request_setenv < set_env.size()) {
        QStringList env = set_env.at(request_setenv).split(QLatin1Char('='));
        if (env.size() != 2) { // broken entry? -- skip rest of the list
            request_setenv = set_env.size();
            break;
        }
        QString name = env.at(0).trimmed().toUpper();
        QString value = env.at(1).trimmed();
        ssh2_method_result = libssh2_channel_setenv(ssh2Channel(), qPrintable(name), qPrintable(value));
        if (ssh2_method_result) {
            if (ssh2_method_result == LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED)
                ssh2_method_result = 0;
            request_setenv = set_env.size();
            break;
        }
        request_setenv++;
    }

    if (!ssh2_method_result) {
        switch (request_terminal) {
        case 1:
            ssh2_method_result = libssh2_channel_request_pty(ssh2Channel(),
                                                             terminal_type.isEmpty() ? "vanilla" :
                                                                 qPrintable(terminal_type));
            if (ssh2_method_result) break;
            request_terminal++;
            [[fallthrough]];
        case 2:
            ssh2_method_result = libssh2_channel_shell(ssh2Channel());
            if (!ssh2_method_result) request_terminal = 1; // get back to original state
            break;
        default:
            ssh2_method_result = libssh2_channel_exec(ssh2Channel(),
                                                      command_.toLocal8Bit().constData());
            break;
        }
    }

    std::error_code error_code = ssh2_success;
    switch (ssh2_method_result) {
    case LIBSSH2_ERROR_EAGAIN:
        setSsh2ProcessState(Starting);
        error_code = Ssh2Error::TryAgain;
        break;
    case 0:
        setSsh2ProcessState(Started);
        break;
    default:
        setSsh2ProcessState(FailedToStart);
        debugSsh2Error(ssh2_method_result);
        error_code = Ssh2Error::ProcessFailedToStart;
        close();
    }
    return error_code;
}

QString Ssh2Process::command() const
{
    return command_;
}

void Ssh2Process::setCommand(const QString &cmd)
{
    TRACE_ARG(cmd);

    if (cmd != command_) {
        command_ = cmd;
        emit commandChanged();
    }
}

QString Ssh2Process::termType() const
{
    return terminal_type;
}

void Ssh2Process::setTermType(const QString &term)
{
    TRACE_ARG(term);

    if (term != terminal_type) {
        terminal_type = term;
        emit termTypeChanged();
    }
}

QStringList Ssh2Process::setEnv() const
{
    return set_env;
}

void Ssh2Process::setSetEnv(const QStringList &env)
{
    TRACE_ARG(env);

    if (env != set_env) {
        set_env = env;
        emit setEnvChanged();
    }
}
