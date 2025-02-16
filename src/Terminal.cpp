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

#include <QRegularExpression>
#include <QDebug>

#include "Parser.h"
#include "Terminal.h"

#ifndef Q_OS_WIN
#include "PtyIFace.h"
#define CONTROL_CODE_ESC '\e'
#else
#define CONTROL_CODE_ESC QChar(0x1b)
#endif

#if defined(Q_OS_MAC)
#    define MyControlModifier Qt::MetaModifier
#else
#    define MyControlModifier Qt::ControlModifier
#endif

#undef qCDebug
#undef qCWarning
#define qCDebug(x)      qDebug()
#define qCWarning(x)    qWarning()

/*!
    \class TerminalLine
    \internal

    A TerminalLine represents a single line of contents in the terminal's buffer.
*/

/*!
    \class TerminalBuffer
    \internal

    A TerminalBuffer is a collection of TerminalLine instances.
*/

static bool charIsHexDigit(QChar ch)
{
    if (ch.isDigit()) // 0-9
        return true;
    else if (ch.toLatin1() >= 65 && ch.toLatin1() <= 70) // A-F
        return true;
    else if (ch.toLatin1() >= 97 && ch.toLatin1() <= 102) // a-f
        return true;

    return false;
}

int TerminalLine::size() const
{
    return m_contents.size();
}

void TerminalLine::append(const TermChar& tc)
{
    m_contents.append(tc);
}

void TerminalLine::insert(int pos, const TermChar& tc)
{
    int i = qBound(0, pos, m_contents.size());
    m_contents.insert(i, tc);
}

void TerminalLine::removeAt(int pos)
{
    if (isIndex(pos)) m_contents.removeAt(pos);
}

void TerminalLine::clear()
{
    m_contents.clear();
}

TermChar& TerminalLine::operator[](int pos)
{
    static TermChar empty;
    return (isIndex(pos) ? m_contents[pos] : empty);
}

const TermChar& TerminalLine::at(int pos) const
{
    static TermChar empty;
    return (isIndex(pos) ? m_contents.at(pos) : empty);
}

bool TerminalLine::isIndex(int pos) const
{
    return (pos >= 0 && pos < m_contents.size());
}

int TerminalBuffer::size() const
{
    return m_buffer.size();
}

void TerminalBuffer::append(const TerminalLine& l)
{
    m_buffer.append(l);
}

void TerminalBuffer::insert(int pos, const TerminalLine& l)
{
    int i = qBound(0, pos, m_buffer.size());
    m_buffer.insert(i, l);
}

void TerminalBuffer::removeAt(int pos)
{
    if (isIndex(pos)) m_buffer.removeAt(pos);
}

void TerminalBuffer::remove(int pos, int count)
{
    if (isIndex(pos) && count > 0) {
        if (pos + count > m_buffer.size()) count = m_buffer.size() - pos;
        if (count > 0) m_buffer.remove(pos, count);
    }
}

TerminalLine TerminalBuffer::takeAt(int pos)
{
    static TerminalLine empty;
    return (isIndex(pos) ? m_buffer.takeAt(pos) : empty);
}

void TerminalBuffer::clear()
{
    m_buffer.clear();
}

TerminalLine& TerminalBuffer::operator[](int pos)
{
    static TerminalLine empty;
    return (isIndex(pos) ? m_buffer[pos] : empty);
}

const TerminalLine& TerminalBuffer::at(int pos) const
{
    static TerminalLine empty;
    return (isIndex(pos) ? m_buffer.at(pos) : empty);
}

bool TerminalBuffer::isIndex(int pos) const
{
    return (pos >= 0 && pos < m_buffer.size());
}

Terminal::Terminal(QObject* parent)
    : QObject(parent)
    , m_pty(nullptr)
    , iTermSize(0, 0)
    , iEmitCursorChangeSignal(true)
    , iShowCursor(true)
    , iUseAltScreenBuffer(false)
    , iAppCursorKeys(false)
    , iReplaceMode(false)
    , iNewLineMode(false)
    , iBackBufferScrollPos(0)
    , back_buffer_size(backBufferSize)
{
    zeroChar.c = ' ';
    zeroChar.bgColor = Parser::fetchDefaultBgColor();
    zeroChar.fgColor = Parser::fetchDefaultFgColor();
    zeroChar.attrib = TermChar::NoAttributes;

    escape = -1;

    iTermAttribs.currentFgColor = Parser::fetchDefaultFgColor();
    iTermAttribs.currentBgColor = Parser::fetchDefaultBgColor();
    iTermAttribs.currentAttrib = TermChar::NoAttributes;
    iTermAttribs.cursorPos = QPoint(0, 0);
    iMarginBottom = 0;
    iMarginTop = 0;

    resetBackBufferScrollPos();
    clearSelection();

    iTermAttribs_saved = iTermAttribs;
    iTermAttribs_saved_alt = iTermAttribs;

    resetTerminal(ResetMode::Hard);
}

bool Terminal::openPty(const QString &shell)
{
#ifdef Q_OS_WIN
    Q_UNUSED(shell);
    return false;
#else
    if (!m_pty) {
        m_pty = new PtyIFace(shell, this);
        if (m_pty->failed()) return false;

        connect(m_pty, &PtyIFace::dataAvailable, this, [this]() {
            insertInBuffer(m_pty->takeText());
        });
        connect(m_pty, &PtyIFace::hangupReceived, this, &Terminal::hangupReceived);
        connect(this, &Terminal::newTerminalChars, m_pty, &PtyIFace::writeText);
    }
    return true;
#endif
}

void Terminal::closePty()
{
    if (m_pty) {
        this->disconnect(m_pty);
        m_pty->disconnect(this);
        m_pty->deleteLater();
        m_pty = nullptr;
    }
}

void Terminal::setCursorPos(QPoint pos)
{
    if (iTermAttribs.cursorPos != pos) {
        int tlimit = 1;
        int blimit = iTermSize.height();
        if (iTermAttribs.originMode) {
            tlimit = iMarginTop;
            blimit = iMarginBottom;
        }

        if (pos.x() < 1)
            pos.setX(1);
        if (pos.x() > iTermSize.width() + 1)
            pos.setX(iTermSize.width() + 1);
        if (pos.y() < tlimit)
            pos.setY(tlimit);
        if (pos.y() > blimit)
            pos.setY(blimit);

        iTermAttribs.cursorPos = pos;
        if (iEmitCursorChangeSignal)
            emit cursorPosChanged(pos);
    }
}

QPoint Terminal::cursorPos() const
{
    return iTermAttribs.cursorPos;
}

bool Terminal::showCursor() const
{
    if (iBackBufferScrollPos != 0)
        return false;

    return iShowCursor;
}

const TerminalBuffer& Terminal::buffer() const
{
    if (iUseAltScreenBuffer)
        return iAltBuffer;

    return iBuffer;
}

TerminalBuffer& Terminal::buffer()
{
    if (iUseAltScreenBuffer)
        return iAltBuffer;

    return iBuffer;
}

QSize Terminal::termSize() const
{
    return iTermSize;
}

void Terminal::setTermSize(const QSize &size)
{
    if (!size.isEmpty() && iTermSize != size) {
        iMarginTop = 1;
        iMarginBottom = size.height();
        iTermSize = size;

        resetTabs();

        emit termSizeChanged(size.width(), size.height());
    }
}

void Terminal::putString(const QString &txt)
{
    if (txt.isEmpty()) return;

    QString str = txt;
    str.replace("\\r", "\r");
    str.replace("\\n", "\n");
    str.replace("\\e", QChar(0x1b));
    str.replace("\\b", "\b");
    str.replace("\\t", "\t");

    //hex
    while (str.indexOf("\\x") != -1) {
        int i = str.indexOf("\\x") + 2;
        QString num;
        while (num.length() < 2 && str.length() > i && charIsHexDigit(str.at(i))) {
            num.append(str.at(i));
            i++;
        }
        str.remove(i - 2 - num.length(), num.length() + 2);
        bool ok;
        str.insert(i - 2 - num.length(), QChar(num.toInt(&ok, 16)));
    }
    //octal
    while (str.indexOf("\\0") != -1) {
        int i = str.indexOf("\\0") + 2;
        QString num;
        while (num.length() < 3 && str.length() > i && (str.at(i).toLatin1() >= 48 && str.at(i).toLatin1() <= 55)) { //accept only 0-7
            num.append(str.at(i));
            i++;
        }
        str.remove(i - 2 - num.length(), num.length() + 2);
        bool ok;
        str.insert(i - 2 - num.length(), QChar(num.toInt(&ok, 8)));
    }

    emit newTerminalChars(str);
}

bool Terminal::keyPress(int key, int modifiers, const QString &text)
{
    QString toWrite;

    if (key > 0xFFFF) {
        int modcode = (modifiers & Qt::ShiftModifier ? 1 : 0) | (modifiers & Qt::AltModifier ? 2 : 0) | (modifiers & MyControlModifier ? 4 : 0);
        switch (key) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if ((modifiers & MyControlModifier) && (modifiers & Qt::ShiftModifier)) toWrite += QChar(0x9E);
            else if (modifiers & MyControlModifier) toWrite += QChar(0x1E); // ^^
            else if (modifiers & Qt::ShiftModifier) toWrite += '\n';
            else if (iNewLineMode) toWrite += "\r\n";
            else toWrite += '\r';
            break;
        case Qt::Key_Backspace:
            if ((modifiers & MyControlModifier) && (modifiers & Qt::ShiftModifier)) toWrite += QChar(0x9F);
            else if (modifiers & MyControlModifier) toWrite += QChar(0x1F); // ^_
            else toWrite += "\x7F";
            break;
        case Qt::Key_Backtab:
            modifiers |= Qt::ShiftModifier;
            [[fallthrough]];
        case Qt::Key_Tab:
            if (modifiers & MyControlModifier) {
                toWrite += CONTROL_CODE_ESC;
                toWrite += QString("[1;5%1I").arg((modifiers & Qt::ShiftModifier) ? 1 : 0);
            } else if (modifiers & Qt::ShiftModifier) {
                toWrite += CONTROL_CODE_ESC;
                toWrite += "[Z";
            } else toWrite += '\t';
            break;
        case Qt::Key_Escape: toWrite += (modifiers & Qt::ShiftModifier) ? QChar(0x9B) : CONTROL_CODE_ESC; break;
        case Qt::Key_Up:    toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1A").arg(modcode) : QString("%1A").arg(iAppCursorKeys ? 'O' : '[')); break;
        case Qt::Key_Down:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1B").arg(modcode) : QString("%1B").arg(iAppCursorKeys ? 'O' : '[')); break;
        case Qt::Key_Right: toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1C").arg(modcode) : QString("%1C").arg(iAppCursorKeys ? 'O' : '[')); break;
        case Qt::Key_Left:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1D").arg(modcode) : QString("%1D").arg(iAppCursorKeys ? 'O' : '[')); break;
        case Qt::Key_PageUp:   toWrite += CONTROL_CODE_ESC + (modcode ? QString("5;1%1~").arg(modcode) : "[5~"); break;
        case Qt::Key_PageDown: toWrite += CONTROL_CODE_ESC + (modcode ? QString("6;1%1~").arg(modcode) : "[6~"); break;
        case Qt::Key_Home:     toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1H").arg(modcode) : "OH"); break;
        case Qt::Key_End:      toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1F").arg(modcode) : "OF"); break;
        case Qt::Key_Insert:   toWrite += CONTROL_CODE_ESC + (modcode ? QString("2;1%1~").arg(modcode) : "[2~"); break;
        case Qt::Key_Delete:   toWrite += CONTROL_CODE_ESC + (modcode ? QString("3;1%1~").arg(modcode) : "[3~"); break;
        case Qt::Key_F1:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1P").arg(modcode) : "OP"); break;
        case Qt::Key_F2:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1Q").arg(modcode) : "OQ"); break;
        case Qt::Key_F3:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1R").arg(modcode) : "OR"); break;
        case Qt::Key_F4:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("1;1%1S").arg(modcode) : "OS"); break;
        case Qt::Key_F5:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("15;1%1~").arg(modcode) : "[15~"); break;
        case Qt::Key_F6:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("17;1%1~").arg(modcode) : "[17~"); break;
        case Qt::Key_F7:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("18;1%1~").arg(modcode) : "[18~"); break;
        case Qt::Key_F8:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("19;1%1~").arg(modcode) : "[19~"); break;
        case Qt::Key_F9:  toWrite += CONTROL_CODE_ESC + (modcode ? QString("20;1%1~").arg(modcode) : "[20~"); break;
        case Qt::Key_F10: toWrite += CONTROL_CODE_ESC + (modcode ? QString("21;1%1~").arg(modcode) : "[21~"); break;
        case Qt::Key_F11: toWrite += CONTROL_CODE_ESC + (modcode ? QString("23;1%1~").arg(modcode) : "[23~"); break;
        case Qt::Key_F12: toWrite += CONTROL_CODE_ESC + (modcode ? QString("24;1%1~").arg(modcode) : "[24~"); break;
        }
    } else {
        QChar ch(key);
        if (ch.isLetter()) ch = (modifiers & Qt::ShiftModifier) ? ch.toUpper() : ch.toLower();
        if (modifiers & Qt::AltModifier) toWrite += CONTROL_CODE_ESC;
        if (modifiers & MyControlModifier) {
            char asciiVal = ch.toUpper().toLatin1(); // Turn uppercase characters into their control code equivalent
            if (asciiVal >= 0x41 && asciiVal <= 0x5f) toWrite += QChar(asciiVal - 0x40);
            else if (!ch.isNull()) qWarning() << "Ctrl+" << ch << "does not translate to control code";
        } else if (text.isEmpty()) toWrite += ch;
        else toWrite += text;
    }
    if (toWrite.isEmpty()) return false;

    resetBackBufferScrollPos();
    if (!toWrite.startsWith(CONTROL_CODE_ESC)) // Only affect selection if not writing escape codes
        clearSelection();

    emit newTerminalChars(toWrite);
    return true;
}

void Terminal::insertInBuffer(const QString& chars)
{
    if (chars.isEmpty() || iTermSize.isNull())
        return;

    iEmitCursorChangeSignal = false;

    for (int i = 0; i < chars.size(); i++) {
        const QChar& ch = chars.at(i);
        auto scalar = ch.toLatin1();

        switch (scalar) {
        case '\n':
        case 11: // vertical tab
        case 12: // form feed
            if (cursorPos().y() == iMarginBottom) {
                scrollFwd(1);
                if (iNewLineMode)
                    setCursorPos(QPoint(1, cursorPos().y()));
            } else if (cursorPos().x() <= columns()) // ignore newline after <termwidth> cols (terminfo: xenl)
            {
                if (iNewLineMode)
                    setCursorPos(QPoint(1, cursorPos().y() + 1));
                else
                    setCursorPos(QPoint(cursorPos().x(), cursorPos().y() + 1));
            }
            break;
        case '\r':
            setCursorPos(QPoint(1, cursorPos().y()));
            break;
        case '\b':
        case 127: // del
            // only move cursor, don't actually erase.
            setCursorPos(QPoint(cursorPos().x() - 1, cursorPos().y()));
            break;
        case '\a':               // BEL
            if (escape == ']') { // BEL also ends OSC sequence
                escape = -1;
                oscSequence(oscSeq);
                oscSeq.clear();
            } else {
                emit visualBell();
            }
            break;
        case '\t':
            forwardTab();
            break;
        case 14: // SI
        case 15: // SO
            // related to character set... ignore
            break;
        default:
            if (escape >= 0) {
                QByteArray escape_set("().*+-/%#");
                if (escape == 0 && (scalar == '[')) {
                    escape = '['; //ansi sequence
                    escSeq += ch;
                } else if (escape == 0 && (scalar == ']')) {
                    escape = ']'; //osc sequence
                    oscSeq += ch;
                } else if (escape == 0 && escape_set.contains(scalar)) {
                    escape = scalar;
                    escSeq += ch;
                } else if (escape == 0 && scalar == '\\') { // ESC\ also ends OSC sequence
                    escape = -1;
                    oscSequence(oscSeq);
                    oscSeq.clear();
                } else if (scalar == CONTROL_CODE_ESC) {
                    escape = 0;
                } else if (escape == '[' || escape_set.contains(escape)) {
                    escSeq += ch;
                } else if (escape == ']') {
                    oscSeq += ch;
                } else if (escape_set.contains(escape)) {
                    escSeq += ch;
                } else {
                    escControlChar(QByteArray(1, scalar));
                    escape = -1;
                }

                if (escape == '[' && scalar >= 64 && scalar <= 126 && scalar != '[') {
                    ansiSequence(escSeq);
                    escape = -1;
                    escSeq.clear();
                }
                if (escape_set.contains(escape) && escSeq.length() >= 2) {
                    escControlChar(escSeq);
                    escape = -1;
                    escSeq.clear();
                }
            } else {
                if (scalar == CONTROL_CODE_ESC) {
                    escape = 0;
                } else {
                    insertAtCursor(ch, !iReplaceMode);
                }
            }
            break;
        }
    }

    iEmitCursorChangeSignal = true;
    emit displayBufferChanged();
}

void Terminal::insertAtCursor(QChar c, bool overwriteMode, bool advanceCursor)
{
    if (cursorPos().x() > iTermSize.width() && advanceCursor) {
        if (iTermAttribs.wrapAroundMode) {
            if (cursorPos().y() >= iMarginBottom) {
                scrollFwd(1);
                setCursorPos(QPoint(1, cursorPos().y()));
            } else {
                setCursorPos(QPoint(1, cursorPos().y() + 1));
            }
        } else {
            setCursorPos(QPoint(iTermSize.width(), cursorPos().y()));
        }
    }

    auto& line = currentLine();
    while (line.size() < cursorPos().x())
        line.append(zeroChar);

    if (!overwriteMode)
        line.insert(cursorPos().x() - 1, zeroChar);

    line[cursorPos().x() - 1].c = c;
    line[cursorPos().x() - 1].fgColor = iTermAttribs.currentFgColor;
    line[cursorPos().x() - 1].bgColor = iTermAttribs.currentBgColor;
    line[cursorPos().x() - 1].attrib = iTermAttribs.currentAttrib;

    if (advanceCursor) {
        setCursorPos(QPoint(cursorPos().x() + 1, cursorPos().y()));
    }
}

void Terminal::eraseLineAtCursor(int from, int to)
{
    if (from == -1 && to == -1) {
        currentLine().clear();
        return;
    }
    if (from < 1)
        from = 1;
    from--;

    if (to < 1 || to > currentLine().size())
        to = currentLine().size();
    to--;

    if (from > to)
        return;

    for (int i = from; i <= to; i++) {
        currentLine()[i] = zeroChar;
    }
}

void Terminal::clearAll(bool wholeBuffer)
{
    clearSelection();
    if (wholeBuffer) {
        backBuffer().clear();
        resetBackBufferScrollPos();
        clearSelection();
    }
    buffer().clear();
    setCursorPos(QPoint(1, 1));
}

void Terminal::backwardTab()
{
    if (cursorPos().y() <= iTabStops.size()) {
        for (int i = iTabStops[cursorPos().y() - 1].count() - 1; i >= 0; i--) {
            if (iTabStops[cursorPos().y() - 1][i] < cursorPos().x()) {
                setCursorPos(QPoint(iTabStops[cursorPos().y() - 1][i], cursorPos().y()));
                break;
            }
        }
    }
}

void Terminal::forwardTab()
{
    if (cursorPos().y() <= iTabStops.size()) {
        for (int i = 0; i < iTabStops[cursorPos().y() - 1].count(); i++) {
            if (iTabStops[cursorPos().y() - 1][i] > cursorPos().x()) {
                setCursorPos(QPoint(iTabStops[cursorPos().y() - 1][i], cursorPos().y()));
                break;
            }
        }
    }
}

static Parser::TextAttributes convert(TermChar::TextAttributes& ta)
{
    Parser::TextAttributes attrs = Parser::NoAttributes;
    if (ta & TermChar::BoldAttribute) {
        attrs |= Parser::TextAttribute::BoldAttribute;
    }
    if (ta & TermChar::ItalicAttribute) {
        attrs |= Parser::TextAttribute::ItalicAttribute;
    }
    if (ta & TermChar::UnderlineAttribute) {
        attrs |= Parser::TextAttribute::UnderlineAttribute;
    }
    if (ta & TermChar::NegativeAttribute) {
        attrs |= Parser::TextAttribute::NegativeAttribute;
    }
    if (ta & TermChar::BlinkAttribute) {
        attrs |= Parser::TextAttribute::BlinkAttribute;
    }
    return attrs;
}

static TermChar::TextAttributes convert(Parser::TextAttributes& ta)
{
    TermChar::TextAttributes attrs = TermChar::NoAttributes;
    if (ta & Parser::BoldAttribute) {
        attrs |= TermChar::TextAttributes(Parser::TextAttribute::BoldAttribute);
    }
    if (ta & Parser::ItalicAttribute) {
        attrs |= TermChar::TextAttributes(Parser::TextAttribute::ItalicAttribute);
    }
    if (ta & Parser::UnderlineAttribute) {
        attrs |= TermChar::TextAttributes(Parser::TextAttribute::UnderlineAttribute);
    }
    if (ta & Parser::NegativeAttribute) {
        attrs |= TermChar::TextAttributes(Parser::TextAttribute::NegativeAttribute);
    }
    if (ta & Parser::BlinkAttribute) {
        attrs |= TermChar::TextAttributes(Parser::TextAttribute::BlinkAttribute);
    }
    return attrs;
}

void Terminal::ansiSequence(const QString& seq)
{
    if (seq.length() <= 1 || seq.at(0) != '[')
        return;

    QChar cmdChar = seq.at(seq.length() - 1);
    QString extra;
    QList<int> params;

    int x = 1;
    while (x < seq.length() - 1 && !QChar(seq.at(x)).isNumber())
        x++;

    QList<QString> tmp = seq.mid(x, seq.length() - x - 1).split(';');
    foreach (QString b, tmp) {
        bool ok = false;
        int t = b.toInt(&ok);
        if (ok) {
            params.append(t);
        }
    }
    if (x > 1)
        extra = seq.mid(1, x - 1);

    bool unhandled = false;

    switch (cmdChar.toLatin1()) {
    case 'A': //cursor up
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(cursorPos().x(), qMax(iMarginTop, cursorPos().y() - params.at(0))));
        break;
    case 'B': //cursor down
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(cursorPos().x(), qMin(iMarginBottom, cursorPos().y() + params.at(0))));
        break;
    case 'C': //cursor fwd
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(qMin(iTermSize.width(), cursorPos().x() + params.at(0)), cursorPos().y()));
        break;
    case 'D': //cursor back
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(qMax(1, cursorPos().x() - params.at(0)), cursorPos().y()));
        break;
    case 'E': //cursor next line
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(1, qMin(iMarginBottom, cursorPos().y() + params.at(0))));
        break;
    case 'F': //cursor prev line
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(1, qMax(iMarginTop, cursorPos().y() - params.at(0))));
        break;
    case 'G': //go to column
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(params.at(0), cursorPos().y()));
        break;
    case 'H': //cursor pos
    case 'f': //cursor pos
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        while (params.count() < 2)
            params.append(1);
        if (iTermAttribs.originMode)
            setCursorPos(QPoint(params.at(1), params.at(0) + iMarginTop - 1));
        else
            setCursorPos(QPoint(params.at(1), params.at(0)));
        break;
    case 'J':
        unhandled = handleDECSED(params, extra);
        break;
    case 'K':
        unhandled = handleEL(params, extra);
        break;
    case 'X':
        unhandled = handleECH(params, extra);
        break;

    case 'I':
        if (!extra.isEmpty() || (params.count() > 1)) {
            unhandled = true;
            break;
        }
        if (params.count() == 0) {
            params.append(1);
        }
        for (int i = 0; i < params.at(0); i++) {
            forwardTab();
        }
        break;

    case 'Z':
        if (!extra.isEmpty() || (params.count() > 1)) {
            unhandled = true;
            break;
        }
        if (params.count() == 0) {
            params.append(1);
        }
        for (int i = 0; i < params.at(0); i++) {
            backwardTab();
        }
        break;

    case 'L': // insert lines
        unhandled = handleIL(params, extra);
        break;
    case 'M': // delete lines
        unhandled = handleDL(params, extra);
        break;
    case 'P': // delete characters
        unhandled = handleDCH(params, extra);
        break;
    case '@': // insert blank characters
        unhandled = handleICH(params, extra);
        break;

    case 'S': // scroll up n lines
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        scrollFwd(params.at(0));
        break;
    case 'T': // scroll down n lines
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        scrollBack(params.at(0));
        break;

    case 'c': // vt100 identification
        if (params.count() == 0)
            params.append(0);
        if (params.count() == 1 && params.at(0) == 0) {
            emit newTerminalChars(QString("%1[?1;2c").arg(CONTROL_CODE_ESC).toLatin1());
        } else {
            unhandled = true;
        }
        break;

    case 'd': //go to row
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 1)
            params.append(1);
        if (params.at(0) == 0)
            params[0] = 1;
        setCursorPos(QPoint(cursorPos().x(), params.at(0)));
        break;

    case 'g': //tab stop manipulation
        if (params.count() == 0)
            params.append(0);
        if (params.at(0) == 0 && extra == "") { //clear tab at current position
            if (cursorPos().y() <= iTabStops.size()) {
                int idx = iTabStops[cursorPos().y() - 1].indexOf(cursorPos().x());
                if (idx != -1)
                    iTabStops[cursorPos().y() - 1].removeAt(idx);
            }
        } else if (params.at(0) == 3 && extra == "") { //clear all tabs
            iTabStops.clear();
        }
        break;

    case 'n':
        if (params.count() >= 1 && params.at(0) == 6 && extra == "") { // write cursor pos
            emit newTerminalChars(QString("%1[%2;%3R").arg(CONTROL_CODE_ESC).arg(cursorPos().y()).arg(cursorPos().x()).toLatin1());
        } else {
            unhandled = true;
        }
        break;

    case 'p':
        if (extra == ">") {
            /* xterm: select X11 visual cursor mode */
            resetTerminal(ResetMode::Soft);
        } else if (extra == "!") {
            /* DECSTR: Soft Reset */
            resetTerminal(ResetMode::Soft);
        } else if (extra == "$") {
            /* DECRQM: Request DEC Private Mode */
            /* If CSI_WHAT is set, then enable, otherwise disable */
            resetTerminal(ResetMode::Soft);
        } else {
            /* DECSCL: Compatibility Level */
            /* Sometimes CSI_DQUOTE is set here, too */
            resetTerminal(ResetMode::Soft);
            // TODO: handle the parameter to figure out which compatibility level to set.
            qCDebug(tunimp) << "unhandled DECSCL" << params << extra;
            unhandled = true;
        }
        break;

    case 's': //save cursor
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        iTermAttribs_saved = iTermAttribs;
        break;
    case 'u': //restore cursor
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        iTermAttribs = iTermAttribs_saved;
        break;

    case 'm': //graphics mode
        if (params.count() == 0)
            params.append(0);
        {
            Parser::TextAttributes attribs = convert(iTermAttribs.currentAttrib);
            Parser::SGRParserState state(iTermAttribs.currentFgColor, iTermAttribs.currentBgColor, Parser::fetchDefaultFgColor(), Parser::fetchDefaultBgColor(), attribs);
            QString errorString;
            if (!Parser::handleSGR(state, params, errorString)) {
                qWarning() << "Error parsing SGR: " << params << extra << " -- " << errorString;
            }
            iTermAttribs.currentAttrib = convert(attribs);
        }
        break;

    case 'h':
        for (int i = 0; i < params.size(); ++i) {
            handleMode(params.at(i), true, extra);
        }
        break;

    case 'l':
        for (int i = 0; i < params.size(); ++i) {
            handleMode(params.at(i), false, extra);
        }
        break;

    case 'q':
        // TODO: DECSCUSR
        // Set cursor style
        // param[0]:
        // - 0/1/not set: blink block
        // - 2: steady block
        // - 3: blink underline
        // - 4: steady underline
        qCDebug(tunimp) << "unhandled DECSCUSR" << params << extra;
        break;

    case 'r': // scrolling region
        if (!extra.isEmpty()) {
            unhandled = true;
            break;
        }
        if (params.count() < 2) {
            while (params.count() < 2)
                params.append(1);
            params[0] = 1;
            params[1] = iTermSize.height();
        }
        if (params.at(0) < 1)
            params[0] = 1;
        if (params.at(1) > iTermSize.height())
            params[1] = iTermSize.height();
        iMarginTop = params.at(0);
        iMarginBottom = params.at(1);
        if (iMarginTop >= iMarginBottom) {
            //invalid scroll region
            if (iMarginTop == iTermSize.height()) {
                iMarginTop = iMarginBottom - 1;
            } else {
                iMarginBottom = iMarginTop + 1;
            }
        }
        setCursorPos(QPoint(1, iMarginTop));
        break;

    case 't':
        // TODO: XTWINOPS, window manipulation.
        // Resize request.
        qCDebug(tunimp) << "unhandled XTWINOPS" << params << extra;
        break;

    default:
        unhandled = true;
        break;
    }

    if (unhandled)
        qCDebug(tunimp) << "unhandled CSI sequence " << cmdChar << params << extra;
}

void Terminal::handleMode(int mode, bool set, const QString& extra)
{
    if (extra == "?") {
        switch (mode) {
        case 1:
            iAppCursorKeys = set;
            break;
        case 5: {
            // inverse video (DECSCNM)
            m_inverseVideoMode = set;
            break;
        }
        case 6: // origin mode
            iTermAttribs.originMode = set;
            break;
        case 7:
            iTermAttribs.wrapAroundMode = set;
            break;
        case 12: // start blinking cursor
            break;
        case 25: // show cursor
            iShowCursor = set;
            break;
        case 1049: // use alt screen buffer and save cursor
            if (set) {
                iTermAttribs_saved_alt = iTermAttribs;
                iUseAltScreenBuffer = true;
                iMarginTop = 1;
                iMarginBottom = iTermSize.height();
                resetBackBufferScrollPos();
                clearSelection();
                clearAll();
            } else {
                iUseAltScreenBuffer = false;
                iTermAttribs = iTermAttribs_saved_alt;
                iMarginBottom = iTermSize.height();
                iMarginTop = 1;
                resetBackBufferScrollPos();
                clearSelection();
            }
            resetTabs();
            emit displayBufferChanged();
            break;
        case 2004: // bracketed paste mode
            m_bracketedPasteMode = set;
            break;
        default:
            qCDebug(tunimp) << "unhandled DEC private mode " << mode << set << extra;
        }
    } else if (extra == "") {
        switch (mode) {
        case 4:
            iReplaceMode = set;
            break;
        case 20:
            iNewLineMode = set;
            break;
        default:
            qCDebug(tunimp) << "Unhandled ANSI mode " << mode << set << extra;
        }
    } else {
        qCDebug(tunimp) << "Unhandled ANSI/DEC mode " << mode << set << extra;
    }
}

bool Terminal::handleIL(const QList<int>& params, const QString& extra)
{
    if (!extra.isEmpty()) {
        qCWarning(tlog) << "IL with unexpected extra" << extra;
        return false;
    }
    if (cursorPos().y() < iMarginTop || cursorPos().y() > iMarginBottom) {
        qCWarning(tlog) << "IL out of bounds: " << cursorPos() << iMarginTop << iMarginBottom;
        return false;
    }

    int p = params.count() < 1 ? 1 : params.at(0);
    if (p == 0) {
        p = 1;
    }

    if (p > iMarginBottom - cursorPos().y()) {
        scrollBack(iMarginBottom - cursorPos().y(), cursorPos().y());
    } else {
        scrollBack(p, cursorPos().y());
    }
    setCursorPos(QPoint(1, cursorPos().y()));
    return false;
}

bool Terminal::handleDL(const QList<int>& params, const QString& extra)
{
    if (!extra.isEmpty()) {
        qCWarning(tlog) << "DL with unexpected extra" << extra;
        return false;
    }
    if (cursorPos().y() < iMarginTop || cursorPos().y() > iMarginBottom) {
        qCWarning(tlog) << "DL out of bounds: " << cursorPos() << iMarginTop << iMarginBottom;
        return false;
    }

    int p = params.count() < 1 ? 1 : params.at(0);
    if (p == 0) {
        p = 1;
    }

    if (p > iMarginBottom - cursorPos().y()) {
        scrollFwd(iMarginBottom - cursorPos().y(), cursorPos().y());
    } else {
        scrollFwd(p, cursorPos().y());
    }
    setCursorPos(QPoint(1, cursorPos().y()));
    return false;
}

bool Terminal::handleDCH(const QList<int>& params, const QString& extra)
{
    if (!extra.isEmpty()) {
        qCWarning(tlog) << "DCH with unexpected extra" << extra;
        return false;
    }

    int p = params.count() < 1 ? 1 : params.at(0);
    if (p == 0) {
        p = 1;
    }

    for (int i = 0; i < p; i++) {
        auto pos = cursorPos();
        if (pos.y() <= 0 || pos.y() >= buffer().size()) {
            // Line out of bounds.
            return false;
        }

        auto& line = buffer()[pos.y() - 1];
        if (pos.x() <= 0 || pos.x() >= line.size()) {
            // Char out of bounds.
            return false;
        }

        line.removeAt(pos.x() - 1);
    }
    return false;
}

bool Terminal::handleICH(const QList<int>& params, const QString& extra)
{
    if (!extra.isEmpty()) {
        qCWarning(tlog) << "ICH with unexpected extra" << extra;
        return false;
    }

    int p = params.count() < 1 ? 1 : params.at(0);
    if (p == 0) {
        p = 1;
    }

    for (int i = 1; i <= p; i++)
        insertAtCursor(zeroChar.c, false, false);
    return true;
}

// Erase in Display (DECSED)
bool Terminal::handleDECSED(const QList<int>& params, const QString& extra)
{
    if (!extra.isEmpty() && extra != "?") {
        qCWarning(tlog) << "DECSED with unexpected extra" << extra;
        return false;
    }

    if (params.count() >= 1 && params.at(0) == 1) {
        eraseLineAtCursor(1, cursorPos().x());
        for (int i = 0; i < cursorPos().y() - 1; i++) {
            buffer()[i].clear();
        }
        return false;
    } else if (params.count() >= 1 && params.at(0) == 2) {
        clearAll();
        return false;
    } else {
        eraseLineAtCursor(cursorPos().x());
        for (int i = cursorPos().y(); i < buffer().size(); i++) {
            buffer()[i].clear();
        }
        return false;
    }

    return true;
}

// Erase in Line (EL)
bool Terminal::handleEL(const QList<int>& params, const QString& extra)
{
    if (!extra.isEmpty() && extra != "?") {
        qCWarning(tlog) << "EL with unexpected extra" << extra;
        return false;
    }
    if (params.count() >= 1 && params.at(0) == 1) {
        eraseLineAtCursor(1, cursorPos().x());
        return false;
    } else if (params.count() >= 1 && params.at(0) == 2) {
        currentLine().clear();
        return false;
    } else {
        eraseLineAtCursor(cursorPos().x());
        return false;
    }
    return true;
}

// Erase Characters (ECH)
bool Terminal::handleECH(const QList<int>& params, const QString& extra)
{
    if (!extra.isEmpty()) {
        qCWarning(tlog) << "ECH with unexpected extra" << extra;
        return false;
    }

    if (params.count() > 1) {
        qCWarning(tlog) << "ECH with unexpected params" << params;
        return false;
    }

    int p = params.isEmpty() ? 1 : params.at(0);
    eraseLineAtCursor(cursorPos().x(), cursorPos().x() + (p ? p - 1 : 0));
    return true;
}

void Terminal::oscSequence(const QString& seq)
{
    if (seq.length() <= 1 || seq.at(0) != ']') {
        qCWarning(tlog) << "unexpected OSC seq" << seq;
        return;
    }

    if (seq.length() >= 3) {
        if (seq.at(0) == ']') {
            if ((seq.at(1) == '0' || seq.at(1) == '2') && seq.at(2) == ';') {
                // set window title
                emit windowTitleChanged(seq.mid(3));
                return;
            } else if (seq.at(1) == '7' && seq.at(2) == ';') {
                // working directory changed
                emit workingDirectoryChanged(seq.mid(3));
                return;
            } else if (seq.at(1) == '6' && seq.at(2) == ';') {
                // iTerm2 proprietary:
                // Set window title and tab chrome background color
                // Ignore for the time being...
                return;
            }
        }
    }

    if (seq.length() >= 4) {
        if (seq.at(0) == ']') {
            if (seq.at(1) == '1' && seq.at(2) == '3' && seq.at(3) == '3' && seq.at(4) == ';') {
                // iTerm2 proprietary(?)
                // Prompt state stuff, related to shell integration
                // Ignore for the time being...
                return;
            }
        }
    }

    if (seq.length() >= 5) {
        if (seq.at(0) == ']') {
            if (seq.at(1) == '1' && seq.at(2) == '3' && seq.at(3) == '3' && seq.at(4) == '7' && seq.at(5) == ';') {
                // iTerm2 proprietary, various stuff, shell integration and more.
                // Ignore for the time being...
                return;
            }
        }
    }

    qCDebug(tunimp) << "unhandled OSC" << seq;
}

void Terminal::escControlChar(const QString& seq)
{
    QChar ch;

    if (seq.length() == 1) {
        ch = seq.at(0);
    } else if (seq.length() > 1) {                // control sequences longer than 1 characters
        if (seq.at(0) == '(' || seq.at(0) == ')') // character set, ignore this for now...
            return;
        if (seq.at(0) == '#' && seq.at(1) == '8') { // test mode, fill screen with 'E'
            clearAll(true);
            for (int i = 0; i < rows(); i++) {
                TerminalLine line;
                for (int j = 0; j < columns(); j++) {
                    TermChar c = zeroChar;
                    c.c = 'E';
                    line.append(c);
                }
                buffer().append(line);
            }
            return;
        }
    }

    if (ch.toLatin1() == '7') { //save cursor
        iTermAttribs_saved = iTermAttribs;
    } else if (ch.toLatin1() == '8') { //restore cursor
        iTermAttribs = iTermAttribs_saved;
    } else if (ch.toLatin1() == '>' || ch.toLatin1() == '=') { //app keypad/normal keypad - ignore these for now...
    }

    else if (ch.toLatin1() == 'H') { // set a tab stop at cursor position
        while (iTabStops.size() < cursorPos().y())
            iTabStops.append(QVector<int>());

        iTabStops[cursorPos().y() - 1].append(cursorPos().x());
        auto& row = iTabStops[cursorPos().y() - 1];
        std::sort(row.begin(), row.end());
    } else if (ch.toLatin1() == 'D') { // cursor down/scroll down one line
        scrollFwd(1, cursorPos().y());
    } else if (ch.toLatin1() == 'M') { // cursor up/scroll up one line
        scrollBack(1, cursorPos().y());
    }

    else if (ch.toLatin1() == 'E') { // new line
        if (cursorPos().y() == iMarginBottom) {
            scrollFwd(1);
            setCursorPos(QPoint(1, cursorPos().y()));
        } else {
            setCursorPos(QPoint(1, cursorPos().y() + 1));
        }
    } else if (ch.toLatin1() == 'c') { // full reset
        resetTerminal(ResetMode::Hard);
    } else if (ch.toLatin1() == 'g') { // visual bell
        emit visualBell();
    } else {
        qCDebug(tunimp) << "unhandled escape code ESC" << seq;
    }
}

TerminalLine& Terminal::currentLine()
{
    while (buffer().size() <= cursorPos().y() - 1)
        buffer().append(TerminalLine());

    if (cursorPos().y() >= 1 && cursorPos().y() <= buffer().size()) {
        return buffer()[cursorPos().y() - 1];
    }

    // we shouldn't get here
    return buffer()[buffer().size() - 1];
}

QStringList Terminal::printableLinesFromCursor(int lines) const
{
    QStringList ret;

    int start = cursorPos().y() - lines;
    int end = cursorPos().y() + lines;

    for (int l = start - 1; l < end; l++) {
        ret.append("");
        if (l >= 0 && l < buffer().size()) {
            for (int i = 0; i < buffer()[l].size(); i++) {
                if (buffer()[l][i].c.isPrint())
                    ret[ret.size() - 1].append(buffer()[l][i].c);
            }
        }
    }

    return ret;
}

void Terminal::scrollBack(int lines, int insertAt)
{
    if (lines <= 0)
        return;

    adjustSelectionPosition(lines);

    bool useBackbuffer = true;
    if (insertAt == -1) {
        insertAt = iMarginTop;
        useBackbuffer = false;
    }
    insertAt--;

    while (lines > 0) {
        if (!iUseAltScreenBuffer) {
            if (iBackBuffer.size() > 0 && useBackbuffer)
                buffer().insert(insertAt, iBackBuffer.takeAt(iBackBuffer.size() - 1));
            else
                buffer().insert(insertAt, TerminalLine());
        } else {
            buffer().insert(insertAt, TerminalLine());
        }

        int rm = iMarginBottom;
        if (rm >= buffer().size())
            rm = buffer().size() - 1;

        buffer().removeAt(rm);

        lines--;
    }
}

void Terminal::scrollFwd(int lines, int removeAt)
{
    if (lines <= 0)
        return;

    adjustSelectionPosition(-lines);

    if (removeAt == -1) {
        removeAt = iMarginTop;
    }
    removeAt--;

    while (buffer().size() < iMarginBottom)
        buffer().append(TerminalLine());

    while (lines > 0) {
        buffer().insert(iMarginBottom, TerminalLine());

        if (!iUseAltScreenBuffer)
            iBackBuffer.append(buffer().takeAt(removeAt));
        else
            buffer().removeAt(removeAt);

        lines--;
    }

    int count = iBackBuffer.size() - back_buffer_size;
    if (count > 0) iBackBuffer.remove(0, count);
}

void Terminal::resetTerminal(ResetMode mode)
{
    // See: https://vt100.net/docs/vt220-rm/chapter4.html
    // 4.18 Terminal Reset (DECSTR and RIS)
    // DECSTR is soft reset mode. RIS is hard.
    if (mode == ResetMode::Hard) {
        iBuffer.clear();
        iAltBuffer.clear();
        iBackBuffer.clear();
        iTermAttribs.cursorPos = QPoint(1, 1);
    }

    iTermAttribs.currentFgColor = Parser::fetchDefaultFgColor();
    iTermAttribs.currentBgColor = Parser::fetchDefaultBgColor();
    iTermAttribs.currentAttrib = TermChar::NoAttributes;
    iTermAttribs.wrapAroundMode = true;
    iTermAttribs.originMode = false;

    iTermAttribs_saved = iTermAttribs;
    iTermAttribs_saved_alt = iTermAttribs;

    iMarginBottom = iTermSize.height();
    iMarginTop = 1;

    iShowCursor = true;
    iUseAltScreenBuffer = false;
    iAppCursorKeys = false;
    iReplaceMode = false;
    iNewLineMode = false;
    m_inverseVideoMode = false;
    m_bracketedPasteMode = false;

    resetBackBufferScrollPos();
    clearSelection();

    resetTabs();
    clearSelection();
}

void Terminal::resetTabs()
{
    iTabStops.clear();
    for (int i = 0; i < iTermSize.height(); i++) {
        int tab = 1;
        iTabStops.append(QVector<int>());
        while (tab <= iTermSize.width()) {
            iTabStops.last().append(tab);
            tab += 8;
        }
    }
}

void Terminal::paste(const QString& text)
{
    if (!text.isEmpty()) {
        resetBackBufferScrollPos();
        clearSelection();

        QString str;
        if (m_bracketedPasteMode) {
            str += CONTROL_CODE_ESC;
            str += "[200~";
        }
        str += text;
        if (m_bracketedPasteMode) {
            str += CONTROL_CODE_ESC;
            str += "[201~";
        }

        emit newTerminalChars(str);
    }
}

QStringList Terminal::grabURLsFromBuffer() const
{
    QStringList ret;
    QByteArray buf;

    //backbuffer
    if (!iUseAltScreenBuffer
        || backBufferScrollPos() > 0) //a lazy workaround: just grab everything when the buffer is being scrolled (TODO: make a proper fix)
    {
        for (int i = 0; i < iBackBuffer.size(); i++) {
            for (int j = 0; j < iBackBuffer[i].size(); j++) {
                if (iBackBuffer[i][j].c.isPrint()) {
                    buf.append(QString(iBackBuffer[i][j].c).toUtf8());
                } else if (iBackBuffer[i][j].c == 0) {
                    buf.append(' ');
                }
            }
            if (iBackBuffer[i].size() < iTermSize.width()) {
                buf.append(' ');
            }
        }
    }

    //main buffer
    for (int i = 0; i < buffer().size(); i++) {
        for (int j = 0; j < buffer()[i].size(); j++) {
            if (buffer()[i][j].c.isPrint()) {
                buf.append(QString(buffer()[i][j].c).toUtf8());
            } else if (buffer()[i][j].c == 0) {
                buf.append(' ');
            }
        }
        if (buffer()[i].size() < iTermSize.width()) {
            buf.append(' ');
        }
    }

    /* http://blog.mattheworiordan.com/post/13174566389/url-regular-expression-for-links-with-or-without-the */
    static QRegularExpression re("("
                          "("                                // brackets covering match for protocol (optional) and domain
                          "([A-Za-z]{3,9}:(?:\\/\\/)?)"      // match protocol, allow in format http:// or mailto:
                          "(?:[\\-;:&=\\+\\$,\\w]+@)?"       // allow something@ for email addresses
                          "[A-Za-z0-9\\.\\-]+"               // anything looking at all like a domain, non-unicode domains
                          "|"                                // or instead of above
                          "(?:www\\.|[\\-;:&=\\+\\$,\\w]+@)" // starting with something@ or www.
                          "[A-Za-z0-9\\.\\-]+"               // anything looking at all like a domain
                          ")"
                          "("                           // brackets covering match for path, query string and anchor
                          "(?:\\/[\\+~%\\/\\.\\w\\-]*)" // allow optional /path
                          "?\\?"
                          "?(?:[\\-\\+=&;%@\\.\\w]*)" // allow optional query string starting with ?
                          "#?(?:[\\.\\!\\/\\\\\\w]*)" // allow optional anchor #anchor
                          ")?"                        // make URL suffix optional
                          ")");

    QRegularExpressionMatchIterator i = re.globalMatch(buf);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(1);
        ret << word;
    }

    ret.removeDuplicates();
    return ret;
}

void Terminal::scrollBackBufferFwd(int lines)
{
    if (lines <= 0)
        return;

    if (iUseAltScreenBuffer) {
        char cursorModif = '[';
        if (iAppCursorKeys)
            cursorModif = 'O';

        QString fmt = QString("%2%1B").arg(cursorModif);
        emit newTerminalChars(fmt.arg(CONTROL_CODE_ESC));
    } else {
        iBackBufferScrollPos -= lines;
        if (iBackBufferScrollPos < 0) {
            iBackBufferScrollPos = 0;
        } else {
            adjustSelectionPosition(-lines);
        }

        emit scrollBackBufferAdjusted(false);
    }
}

void Terminal::scrollBackBufferBack(int lines)
{
    if (lines <= 0)
        return;

    if (iUseAltScreenBuffer) {
        char cursorModif = '[';
        if (iAppCursorKeys)
            cursorModif = 'O';

        QString fmt = QString("%2%1A").arg(cursorModif);
        emit newTerminalChars(fmt.arg(CONTROL_CODE_ESC));
    } else {
        iBackBufferScrollPos += lines;
        if (iBackBufferScrollPos > iBackBuffer.size()) {
            iBackBufferScrollPos = iBackBuffer.size();
        } else {
            adjustSelectionPosition(lines);
        }

        emit scrollBackBufferAdjusted(false);
    }
}

void Terminal::resetBackBufferScrollPos()
{
    if (iBackBufferScrollPos == 0 && iSelection.isNull())
        return;

    if (!iUseAltScreenBuffer) {
        scrollBackBufferFwd(iBackBufferScrollPos);
    } else {
        iBackBufferScrollPos = 0;
        emit scrollBackBufferAdjusted(true);
    }
}

QString Terminal::selectedText() const
{
    if (selection().isNull())
        return QString();

    QString text;
    QString line;

    // backbuffer
    if (iBackBufferScrollPos > 0 && !iUseAltScreenBuffer) {
        int lineFrom = iBackBuffer.size() - iBackBufferScrollPos + selection().top() - 1;
        int lineTo = iBackBuffer.size() - iBackBufferScrollPos + selection().bottom() - 1;

        for (int i = lineFrom; i <= lineTo; i++) {
            if (i >= 0 && i < iBackBuffer.size()) {
                line.clear();
                int start = 0;
                int end = iBackBuffer[i].size() - 1;
                if (i == lineFrom) {
                    start = selection().left() - 1;
                }
                if (i == lineTo) {
                    end = selection().right() - 1;
                }
                for (int j = start; j <= end; j++) {
                    if (j >= 0 && j < iBackBuffer[i].size() && iBackBuffer[i][j].c.isPrint())
                        line += iBackBuffer[i][j].c;
                }
                text += line.trimmed() + "\n";
            }
        }
    }

    // main buffer
    int lineFrom = selection().top() - 1 - iBackBufferScrollPos;
    int lineTo = selection().bottom() - 1 - iBackBufferScrollPos;
    for (int i = lineFrom; i <= lineTo; i++) {
        if (i >= 0 && i < buffer().size()) {
            line.clear();
            int start = 0;
            int end = buffer()[i].size() - 1;
            if (i == lineFrom) {
                start = selection().left() - 1;
            }
            if (i == lineTo) {
                end = selection().right() - 1;
            }
            for (int j = start; j <= end; j++) {
                if (j >= 0 && j < buffer()[i].size() && buffer()[i][j].c.isPrint())
                    line += buffer()[i][j].c;
            }
            text += line.trimmed() + "\n";
        }
    }

    return text.trimmed();
}

void Terminal::adjustSelectionPosition(int lines)
{
    // adjust selection position when terminal contents move

    if (iSelection.isNull() || lines == 0)
        return;

    int tx = iSelection.left();
    int ty = iSelection.top() + lines;
    int bx = iSelection.right();
    int by = iSelection.bottom() + lines;

    if (ty < 1) {
        ty = 1;
        tx = 1;
    }
    if (by > iTermSize.height()) {
        by = iTermSize.height();
        bx = iTermSize.width();
    }
    if (by < 1 || ty > iTermSize.height()) {
        clearSelection();
        return;
    }

    iSelection = QRect(QPoint(tx, ty), QPoint(bx, by));

    emit selectionChanged();
}

void Terminal::setSelection(QPoint start, QPoint end, bool selectionOngoing)
{
    if (start.y() > end.y())
        qSwap(start, end);
    if (start.y() == end.y() && start.x() > end.x())
        qSwap(start, end);

    if (start.x() < 1)
        start.rx() = 1;
    if (start.y() < 1)
        start.ry() = 1;
    if (end.x() > iTermSize.width())
        end.rx() = iTermSize.width();
    if (end.y() > iTermSize.height())
        end.ry() = iTermSize.height();

    iSelection = QRect(start, end);

    emit selectionChanged();

    if (!selectionOngoing) {
        emit selectionFinished();
    }
}

void Terminal::clearSelection()
{
    if (iSelection.isNull())
        return;

    iSelection = QRect();

    emit selectionFinished();
    emit selectionChanged();
}

int Terminal::rows() const
{
    return iTermSize.height();
}

int Terminal::columns() const
{
    return iTermSize.width();
}

QRect Terminal::selection() const
{
    return iSelection;
}
