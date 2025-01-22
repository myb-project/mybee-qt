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

#ifndef PTYIFACE_H
#define PTYIFACE_H

#include <QObject>
#include <QByteArray>

class QSocketNotifier;
class QTextCodec;
class Terminal;

// Please use correct Charset for the pty interface! UTF-8 by default

class PtyIFace : public QObject
{
    Q_OBJECT
public:
    static constexpr char const *defaultCharset   = "UTF-8";
    static constexpr char const *defaultTermType  = "xterm-256color";

    PtyIFace(const QString &charset, const QString &term, const QString &shell, Terminal *parent);
    PtyIFace(const QString &shell, Terminal *parent)
        : PtyIFace(defaultCharset, defaultTermType, shell, parent) {}
    ~PtyIFace();

    bool failed() const { return iFailed; }
    QString takeText(); // return Unicode string

public slots:
    void writeText(const QString &chars); // input string must be Unicode

private slots:
    void resize(int columns, int rows);

signals:
    void dataAvailable();
    void hangupReceived();

private slots:
    void checkForDeadPids();
    void onSocketRead();

private:
    Q_DISABLE_COPY(PtyIFace)

    int iPid;
    int iMasterFd;
    bool iFailed;
    bool m_childProcessQuit;
    int m_childProcessPid;

    QSocketNotifier *iReadNotifier;
    QByteArray pending_data;

    QTextCodec *iTextCodec;


    static void sighandler(int sig);
    static std::vector<int> m_deadPids;
    static bool m_initializedSignalHandler;
};

#endif // PTYIFACE_H
