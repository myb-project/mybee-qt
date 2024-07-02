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

#include <QMetaType>
#include <QString>

#include <system_error>

struct _LIBSSH2_SESSION;
typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;

struct _LIBSSH2_CHANNEL;
typedef struct _LIBSSH2_CHANNEL LIBSSH2_CHANNEL;

struct _LIBSSH2_KNOWNHOSTS;
typedef struct _LIBSSH2_KNOWNHOSTS LIBSSH2_KNOWNHOSTS;

enum Ssh2Error {
    ErrorReadKnownHosts = 1,
    ErrorWriteKnownHosts,
    SessionStartupError,
    UnexpectedError,
    HostKeyInvalidError,
    HostKeyMismatchError,
    HostKeyUnknownError,
    HostKeyAppendError,
    AuthenticationError,
    FailedToOpenChannel,
    FailedToCloseChannel,
    ProcessFailedToStart,
    ChannelReadError,
    ChannelWriteError,
    TryAgain,
    ConnectionTimeoutError,
    TcpConnectionError,
    TcpConnectionRefused,
    ScpReadFileError,
    ScpWriteFileError
};

std::error_code make_error_code(Ssh2Error ssh2_error);

extern const std::error_code ssh2_success;

inline bool checkSsh2Error(const std::error_code& error_code) {
    return (error_code == ssh2_success || error_code.value() == Ssh2Error::TryAgain);
}

#define debugSsh2Error(x) //qDebug() << "Ssh2 error: " << a

namespace std
{
template <>
struct is_error_code_enum<Ssh2Error> : true_type {
};
} // namespace std
