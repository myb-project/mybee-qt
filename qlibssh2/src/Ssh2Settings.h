/*
MIT License

Copyright (c) 2023 Vladimir Vorobyev <b800xy@gmail.com>

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

#include <QMetaType>
#include <QSharedDataPointer>

class Ssh2SettingsData;

class Ssh2Settings
{
public:
    static constexpr char const *defaulHost   = "localhost";
    static constexpr int const defaultPort    = 22;
    static constexpr int const defaultTimeout = 3000; // milliseconds

    Ssh2Settings();
    Ssh2Settings(const QString &user, const QString &host, quint16 port = defaultPort);
    Ssh2Settings(const Ssh2Settings &other);
    Ssh2Settings &operator=(const Ssh2Settings &other);
    ~Ssh2Settings();

    bool operator==(const Ssh2Settings &other) const;
    bool operator!=(const Ssh2Settings &other) const;

    bool isValid() const;
    bool isPasswordAuth() const;
    bool isKeyAuth() const;

    QString user() const;
    QString host() const;
    quint16 port() const;
    QString passPhrase() const;
    QString key() const;
    QString keyPhrase() const;
    QString knownHosts() const;
    uint timeout() const;
    QString autoAppendToKnownHosts() const;
    bool checkServerIdentity() const;

    void setUser(const QString &value);
    void setHost(const QString &value);
    void setPort(quint16 value);
    void setPassPhrase(const QString &value);
    void setKey(const QString &value);
    void setKeyPhrase(const QString &value);
    void setKnownHosts(const QString &value);
    void setTimeout(uint value);
    void setAutoAppendKnownHost(const QString &value);
    void setCheckServerIdentity(bool enable);

    friend inline QDebug& operator<<(QDebug &dbg, const Ssh2Settings &from) {
        from.dump(dbg); return dbg; }

private:
    void dump(QDebug &dbg) const;

    QSharedDataPointer<Ssh2SettingsData> d;
};

Q_DECLARE_TYPEINFO(Ssh2Settings, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(Ssh2Settings)
