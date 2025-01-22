/*
    Copyright (c) 2023 Vladimir Vorobyev <b800xy@gmail.com>
    Copyright (C) 2017 Robin Burchell <robin+git@viroteck.net>
    Copyright 2011-2012 Heikki Holstila <heikki.holstila@gmail.com>

    This work is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This work is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this work.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QSocketNotifier>
#include <QTextCodec>
#include <QDebug>

extern "C" {
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#if defined(Q_OS_LINUX)
#include <pty.h>
#elif defined(Q_OS_MAC)
#include <util.h>
#elif defined(Q_OS_FREEBSD)
#include <termios.h>
#include <libutil.h>
#endif
}

#include "PtyIFace.h"
#include "Terminal.h"

std::vector<int> PtyIFace::m_deadPids;
bool PtyIFace::m_initializedSignalHandler = false;

void PtyIFace::sighandler(int sig)
{
    if (sig == SIGCHLD) {
        int pid = ::wait(NULL);
        if (pid > 0) {
            // we cannot reallocate in a signal handler, or Bad Things will happen
            Q_ASSERT(m_deadPids.size() + 1 <= m_deadPids.capacity());
            m_deadPids.push_back(pid);
        }
    }
}

void PtyIFace::checkForDeadPids()
{
    for (size_t i = 0; i < m_deadPids.size(); ++i) {
        if (m_deadPids.at(i) == m_childProcessPid) {
            iReadNotifier->setEnabled(false);
            m_deadPids.erase(m_deadPids.begin() + i);
            int status = 0;
            ::waitpid(m_childProcessPid, &status, WNOHANG);
            m_childProcessQuit = true;
            m_childProcessPid = 0;
            emit hangupReceived();
            break;
        }
    }
}

PtyIFace::PtyIFace(const QString &charset, const QString &term, const QString &shell, Terminal *parent)
    : QObject(parent)
    , iPid(0)
    , iMasterFd(0)
    , iFailed(false)
    , m_childProcessQuit(false)
    , m_childProcessPid(0)
    , iReadNotifier(nullptr)
    , iTextCodec(nullptr)
{
    Q_ASSERT(parent);

    m_deadPids.reserve(m_deadPids.capacity() + 1);
    connect(QCoreApplication::eventDispatcher(), &QAbstractEventDispatcher::awake,
            this, &PtyIFace::checkForDeadPids);

    // fork the child process before creating QGuiApplication
    int socketM;
    int pid = ::forkpty(&socketM, NULL, NULL, NULL);
    if (pid == -1) {
        iFailed = true;
        qWarning() << Q_FUNC_INFO << "forkpty" << ::strerror(errno);
        return;
    }
    if (pid == 0) {
        if (!term.isEmpty())
             qputenv("TERM", term.toLatin1());
        else qputenv("TERM", defaultTermType);

        QString execCmd = shell;
        if (execCmd.isEmpty()) { // execute the user's default shell
            const struct passwd *pwd = ::getpwuid(::getuid());
            execCmd = (pwd && pwd->pw_shell) ? pwd->pw_shell : "/bin/sh";
            execCmd.append(" -l"); // -l is typical for all shells, unlike --login
        }
        QStringList execParts = execCmd.split(' ', Qt::SkipEmptyParts);
        if (execParts.isEmpty()) {
            qWarning() << Q_FUNC_INFO << "Command is empty, nothing to execute!";
            ::exit(0);
        }
        char *ptrs[100];
        for (int i = 0; i < execParts.length() && i < 99; i++) {
            ptrs[i] = new char[execParts.at(i).toLatin1().length() + 1];
            ::memcpy(ptrs[i], execParts.at(i).toLatin1().data(), execParts.at(i).toLatin1().length());
            ptrs[i][execParts.at(i).toLatin1().length()] = 0;
        }
        ptrs[execParts.length()] = 0;

        ::execvp(execParts.first().toLatin1(), ptrs);
        ::exit(0);
    }

    iPid = pid;
    iMasterFd = socketM;
    m_childProcessPid = iPid;

    if (m_childProcessQuit) {
        iFailed = true;
        qWarning() << Q_FUNC_INFO << "The terminal is unavailable";
        return;
    }

    resize(parent->columns(), parent->rows());
    connect(parent, &Terminal::termSizeChanged, this, &PtyIFace::resize);

    iReadNotifier = new QSocketNotifier(iMasterFd, QSocketNotifier::Read, this);
    connect(iReadNotifier, &QSocketNotifier::activated, this, &PtyIFace::onSocketRead);

    if (!m_initializedSignalHandler) {
        ::signal(SIGCHLD, &PtyIFace::sighandler);
        m_initializedSignalHandler = true;
    }
    ::fcntl(iMasterFd, F_SETFL, O_NONBLOCK); // reads from the descriptor should be non-blocking

    if (!charset.isEmpty())
         iTextCodec = QTextCodec::codecForName(charset.toLatin1());
    else iTextCodec = QTextCodec::codecForName(defaultCharset);
}

PtyIFace::~PtyIFace()
{
    if (!m_childProcessQuit) { // make the process quit
        ::kill(iPid, SIGHUP);
        ::kill(iPid, SIGTERM);
    }
}

void PtyIFace::resize(int columns, int rows)
{
    if (m_childProcessQuit) return;

    winsize winp;
    winp.ws_col = columns;
    winp.ws_row = rows;
    ::ioctl(iMasterFd, TIOCSWINSZ, &winp);
}

void PtyIFace::onSocketRead()
{
    if (m_childProcessQuit) return;

    char buf[1024];
    int len = ::read(iMasterFd, buf, sizeof(buf));
    if (len < 1) return;

    if (pending_data.size() + len >= int(sizeof(buf)) * 1024)
        pending_data.clear(); // the old data will be lost because no one reads it for a long time

    pending_data.append(buf, len);

    emit dataAvailable();
}

void PtyIFace::writeText(const QString &chars)
{
    if (chars.isEmpty() || m_childProcessQuit) return;

    const QByteArray data = iTextCodec ? iTextCodec->fromUnicode(chars) : chars.toLocal8Bit();
    if (data.size()) {
        int ret = ::write(iMasterFd, data, data.size());
        if (ret != data.size())
            qDebug() << Q_FUNC_INFO << "write error!";
    }
}

QString PtyIFace::takeText()
{
    QString text;
    if (pending_data.size()) {
        text = iTextCodec ? iTextCodec->toUnicode(pending_data) : QString::fromLocal8Bit(pending_data);
        pending_data.clear();
    }
    return text;
}
