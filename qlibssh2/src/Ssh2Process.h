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
#pragma once

#include <QIODevice>

#include "Ssh2Channel.h"

class Ssh2Process : public Ssh2Channel
{
    Q_OBJECT
    //Q_ENUMS(ProcessStates)
    Q_PROPERTY(ProcessStates processState READ processState NOTIFY processStateChanged)
    Q_PROPERTY(QString     command READ command  WRITE setCommand  NOTIFY commandChanged)
    Q_PROPERTY(QString    termType READ termType WRITE setTermType NOTIFY termTypeChanged)
    Q_PROPERTY(QStringList  setEnv READ setEnv   WRITE setSetEnv   NOTIFY setEnvChanged)

public:
    ~Ssh2Process();

    enum ProcessStates {
        NotStarted,
        Starting,
        FailedToStart,
        Started,
        Finishing,
        Finished
    };

    ProcessStates processState() const;
    void checkIncomingData() override;

    QString command() const;
    void setCommand(const QString &cmd);

    QString termType() const;
    void setTermType(const QString &term);

    QStringList setEnv() const;
    void setSetEnv(const QStringList &env);

signals:
    void processStateChanged(ProcessStates processState);
    void commandChanged();
    void termTypeChanged();
    void setEnvChanged();

protected:
    Ssh2Process(const QString &command, Ssh2Client *ssh2_client);

private slots:
    void onSsh2ChannelStateChanged(const ChannelStates& state);

private:
    void setSsh2ProcessState(ProcessStates ssh2_process_state);
    std::error_code execSsh2Process();

    QString command_;
    ProcessStates ssh2_process_state_{ProcessStates::NotStarted};

    int request_setenv;
    int request_terminal;
    QString terminal_type;
    QStringList set_env;

    friend class Ssh2Client;
};
