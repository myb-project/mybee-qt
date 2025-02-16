/*
    Copyright (c) 2023 Vladimir Vorobyev <b800xy@gmail.com>
    Copyright (C) 2017 Crimson AS <info@crimson.no>
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

#include <QClipboard>
#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QStyleHints>
#include <QTimer>
#include <cmath>

#include "Parser.h"
#include "Terminal.h"
#include "TextRender.h"
#include "SshSession.h"
#include "SshChannelShell.h"

//#define TRACE_TEXTRENDER
#ifdef  TRACE_TEXTRENDER
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

/*!
 * \internal
 *
 * TextRender is a QQuickItem that acts as a view for data from a Terminal,
 * and serves as an interaction point with a Terminal.
 *
 * TextRender is organized of a number of different parts. The user is expected
 * to set a number of "delegates", which are the pieces instantiated by
 * TextRender to correspond with the data from the Terminal. For instance, there
 * is a background cell delegate (for coloring), a cell contents delegate (for
 * the text), a cursor delegate, and so on.
 *
 * TextRender organises its child delegate instances in a slightly complex way,
 * due to the amount of items it manages, and the requirements involved:
 *
 * TextRender
 *      contentItem
 *          backgroundContainer
 *              cellDelegates
 *          textContainer
 *              cellContentsDelegates
 *          overlayContainer
 *              cursorDelegate
 *              selectionDelegates
 *
 * The contentItem is separate from TextRender itself so that contentItem can
 * have visual effects applied (like a <1.0 opacity) without affecting items
 * that are placed inside TextRender on the user's side. This is used in the
 * mobile UX for instance, where the keyboard is placed inside TextRender, and
 * opacity on the keyboard and TextRender's contentItem are swapped when the
 * keyboard transitions to and from active state.
 */

TextRender::TextRender(QQuickItem* parent)
    : QQuickItem(parent)
    , m_activeClick(false)
    , m_touchMoved(false)
    , m_modifiers(0)
    , iFont(QLatin1String("monospace"), QGuiApplication::font().pointSize() - 1)
    , iShowBufferScrollIndicator(false)
    , iAllowGestures(true)
    , last_selected(false)
    , m_contentItem(0)
    , m_backgroundContainer(0)
    , m_textContainer(0)
    , m_overlayContainer(0)
    , m_cellDelegate(0)
    , m_cellContentsDelegate(0)
    , m_cursorDelegate(0)
    , m_cursorDelegateInstance(0)
    , m_selectionDelegate(0)
    , m_topSelectionDelegateInstance(0)
    , m_middleSelectionDelegateInstance(0)
    , m_bottomSelectionDelegateInstance(0)
    , m_dragMode(DragGestures)
    , m_dispatch_timer(nullptr)
    , m_mouse_timer(nullptr)
{
    TRACE();
    setFontMetrics();

    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setCursor(Qt::IBeamCursor);
    setFlag(QQuickItem::ItemAcceptsInputMethod, true);
    setFlag(QQuickItem::ItemIsFocusScope, true);
    //setFlag(QQuickItem::ItemHasContents, true);
    setFocusPolicy(Qt::StrongFocus);

    connect(this, &QQuickItem::widthChanged, this, &TextRender::redraw);
    connect(this, &QQuickItem::heightChanged, this, &TextRender::redraw);

    connect(&m_terminal, &Terminal::windowTitleChanged, this, &TextRender::handleTitleChanged);
    connect(&m_terminal, &Terminal::visualBell, this, &TextRender::visualBell);
    connect(&m_terminal, &Terminal::hangupReceived, this, &TextRender::hangupReceived);
    connect(&m_terminal, &Terminal::displayBufferChanged, this, &TextRender::redraw);
    connect(&m_terminal, &Terminal::displayBufferChanged, this, &TextRender::displayBufferChanged);
    connect(&m_terminal, SIGNAL(cursorPosChanged(QPoint)), this, SLOT(redraw()));
    connect(&m_terminal, SIGNAL(termSizeChanged(int,int)), this, SIGNAL(terminalSizeChanged()));
    connect(&m_terminal, &Terminal::selectionChanged, this, &TextRender::redraw);
    connect(&m_terminal, &Terminal::scrollBackBufferAdjusted, this, &TextRender::handleScrollBack);
    connect(&m_terminal, &Terminal::selectionChanged, this, [this]() {
        bool yes = !m_terminal.selection().isNull();
        if (yes != last_selected) {
            last_selected = yes;
            emit selectedChanged();
        }
    });
}

TextRender::~TextRender()
{
    TRACE();
}

QStringList TextRender::printableLinesFromCursor(int lines) const
{
    return m_terminal.printableLinesFromCursor(lines);
}

void TextRender::putString(const QString &str)
{
    TRACE_ARG(str);
    if (!str.isEmpty()) m_terminal.putString(str);
}

QStringList TextRender::grabURLsFromBuffer() const
{
    return m_terminal.grabURLsFromBuffer();
}

void TextRender::componentComplete()
{
    QQuickItem::componentComplete();

    if (ssh_session) {
        QSize size((width() - 4) / iFontWidth, (height() - 4) / iFontHeight);
        m_terminal.setTermSize(size);
        ssh_shell = ssh_session->createShell(size);
        if (!ssh_shell) {
            qWarning() << Q_FUNC_INFO << "createShell() return null pointer";
            return;
        }
        auto shell = ssh_shell.toStrongRef().data();
        connect(shell, &SshChannelShell::channelOpened, this, &TextRender::terminalReady, Qt::QueuedConnection);
        connect(shell, &SshChannel::channelClosed, this, &TextRender::hangupReceived);

        connect(shell, &SshChannelShell::textReceived, this, [this](const QString &text) {
            //TRACE_ARG(text);
            if (text.count('\n') != text.count('\r'))
                m_terminal.insertInBuffer(QString(text).replace(QLatin1Char('\n'), QStringLiteral("\r\n")));
            else m_terminal.insertInBuffer(text);
        });
        connect(&m_terminal, &Terminal::newTerminalChars, this, [this](const QString &text) {
            //TRACE_ARG(text);
            if (ssh_shell) ssh_shell.toStrongRef()->sendText(text);
        });
        connect(&m_terminal, &Terminal::termSizeChanged, shell, &SshChannelShell::setTermSize);

    } else if (m_terminal.openPty()) {
        emit terminalReady();
    } else {
        handleTitleChanged(tr("Pseudo TTY unavailable"));
    }
}

bool TextRender::selected() const
{
    return last_selected;
}

bool TextRender::copy()
{
    if (!last_selected) return false;
    QClipboard* cb = QGuiApplication::clipboard();
    if (!cb) return false;
    cb->clear();
    QString text = selectedText();
    if (text.isEmpty()) return false;
    cb->setText(text);
    return true;
}

bool TextRender::paste()
{
    QClipboard* cb = QGuiApplication::clipboard();
    if (!cb) return false;
    QString text = cb->text();
    if (text.isEmpty()) return false;
    m_terminal.paste(text);
    return true;
}

void TextRender::deselect()
{
    m_terminal.clearSelection();
}

QString TextRender::selectedText() const
{
    return m_terminal.selectedText();
}

QSize TextRender::terminalSize() const
{
    return QSize(m_terminal.columns(), m_terminal.rows());
}

QString TextRender::title() const
{
    return m_title;
}

void TextRender::handleTitleChanged(const QString& newTitle)
{
    if (m_title == newTitle)
        return;

    m_title = newTitle;
    emit titleChanged();
}

TextRender::DragMode TextRender::dragMode() const
{
    return m_dragMode;
}

void TextRender::setDragMode(DragMode dragMode)
{
    TRACE_ARG(dragMode);
    if (m_dragMode != dragMode) {
        m_dragMode = dragMode;
        if (m_dragMode != DragSelect)
            m_terminal.clearSelection();
        emit dragModeChanged();
    }
}

void TextRender::setContentItem(QQuickItem* contentItem)
{
    Q_ASSERT(!m_contentItem); // changing this requires work
    m_contentItem = contentItem;
    m_contentItem->setParentItem(this);
    m_backgroundContainer = new QQuickItem(m_contentItem);
    m_backgroundContainer->setClip(true);
    m_textContainer = new QQuickItem(m_contentItem);
    m_textContainer->setClip(true);
    m_overlayContainer = new QQuickItem(m_contentItem);
    m_overlayContainer->setClip(true);
    polish();
}

void TextRender::setFont(const QFont& font)
{
    if (iFont == font)
        return;

    iFont = font;
    setFontMetrics();

    polish();
    emit fontChanged();
    emit cellSizeChanged();
}

QFont TextRender::font() const
{
    return iFont;
}

void TextRender::setFontMetrics()
{
    QFontMetricsF fontMetrics(iFont);

    iFontHeight = fontMetrics.height();
    iFontWidth = fontMetrics.horizontalAdvance(' ');
    iFontDescent = fontMetrics.descent();
}

bool TextRender::showBufferScrollIndicator() const
{
    return iShowBufferScrollIndicator;
}

void TextRender::setShowBufferScrollIndicator(bool on)
{
    if (iShowBufferScrollIndicator != on) {
        iShowBufferScrollIndicator = on;
        emit showBufferScrollIndicatorChanged();
    }
}

/*! \internal
 *
 * Fetch a cell from the free list (or allocate a new one, if required)
 */
QQuickItem* TextRender::fetchFreeCell()
{
    QQuickItem* it = nullptr;
    if (!m_freeCells.isEmpty()) {
        it = m_freeCells.takeFirst();
    } else {
        it = qobject_cast<QQuickItem*>(m_cellDelegate->create(qmlContext(this)));
    }

    it->setParentItem(m_backgroundContainer);
    m_cells.append(it);
    return it;
}

/*! \internal
 *
 * Fetch a content cell from the free list (or allocate a new one, if required)
 */
QQuickItem* TextRender::fetchFreeCellContent()
{
    QQuickItem* it = nullptr;
    if (!m_freeCellsContent.isEmpty()) {
        it = m_freeCellsContent.takeFirst();
    } else {
        it = qobject_cast<QQuickItem*>(m_cellContentsDelegate->create(qmlContext(this)));
    }

    it->setParentItem(m_textContainer);
    m_cellsContent.append(it);
    return it;
}

void TextRender::updatePolish()
{
    TRACE();

    // ### these should be handled more carefully
    emit contentYChanged();
    emit visibleHeightChanged();
    emit contentHeightChanged();

    // Make sure the terminal's size is right
    m_terminal.setTermSize(QSize((width() - 4) / iFontWidth, (height() - 4) / iFontHeight));

    if (!m_contentItem || m_terminal.rows() == 0 || m_terminal.columns() == 0)
        return;

    m_contentItem->setWidth(width());
    m_contentItem->setHeight(height());
    m_backgroundContainer->setWidth(width());
    m_backgroundContainer->setHeight(height());
    m_textContainer->setWidth(width());
    m_textContainer->setHeight(height());
    m_overlayContainer->setWidth(width());
    m_overlayContainer->setHeight(height());

    // Push everything back to the free list.
    // We could optimize this by having a "dirty" area from the terminal backend.
    m_freeCells += m_cells;
    m_freeCellsContent += m_cellsContent;

    m_cells.clear();
    m_cellsContent.clear();

    qreal y = 0;
    int yDelegateIndex = 0;
    if (m_terminal.backBufferScrollPos() != 0 && m_terminal.backBuffer().size() > 0) {
        int from = m_terminal.backBuffer().size() - m_terminal.backBufferScrollPos();
        if (from < 0)
            from = 0;
        int to = m_terminal.backBuffer().size();
        if (to - from > m_terminal.rows())
            to = from + m_terminal.rows();
        paintFromBuffer(m_terminal.backBuffer(), from, to, y, yDelegateIndex);
        if ((to - from) < m_terminal.rows() && m_terminal.buffer().size() > 0) {
            int to2 = m_terminal.rows() - (to - from);
            if (to2 > m_terminal.buffer().size())
                to2 = m_terminal.buffer().size();
            paintFromBuffer(m_terminal.buffer(), 0, to2, y, yDelegateIndex);
        }
    } else {
        int count = qMin(m_terminal.rows(), m_terminal.buffer().size());
        paintFromBuffer(m_terminal.buffer(), 0, count, y, yDelegateIndex);
    }

    // any remaining items in the free lists are unused
    for (int i = 0; i < m_freeCells.size(); i++) {
        m_freeCells.at(i)->setVisible(false);
    }
    for (int i = 0; i < m_freeCellsContent.size(); i++) {
        m_freeCellsContent.at(i)->setVisible(false);
    }

    // cursor
    if (m_terminal.showCursor()) {
        if (!m_cursorDelegateInstance) {
            m_cursorDelegateInstance = qobject_cast<QQuickItem*>(m_cursorDelegate->create(qmlContext(this)));
            m_cursorDelegateInstance->setVisible(false);
            m_cursorDelegateInstance->setParentItem(m_overlayContainer);
        }

        m_cursorDelegateInstance->setVisible(true);
        QPointF cursor = cursorPixelPos();
        QSizeF csize = cellSize();
        m_cursorDelegateInstance->setX(cursor.x());
        m_cursorDelegateInstance->setY(cursor.y());
        m_cursorDelegateInstance->setWidth(csize.width());
        m_cursorDelegateInstance->setHeight(csize.height());
        m_cursorDelegateInstance->setProperty("color", Parser::fetchDefaultFgColor());
    } else if (m_cursorDelegateInstance) {
        m_cursorDelegateInstance->setVisible(false);
    }

    QRect selection = m_terminal.selection();
    if (!selection.isNull()) {
        if (!m_topSelectionDelegateInstance) {
            m_topSelectionDelegateInstance = qobject_cast<QQuickItem*>(m_selectionDelegate->create(qmlContext(this)));
            m_topSelectionDelegateInstance->setVisible(false);
            m_topSelectionDelegateInstance->setParentItem(m_overlayContainer);

            m_middleSelectionDelegateInstance = qobject_cast<QQuickItem*>(m_selectionDelegate->create(qmlContext(this)));
            m_middleSelectionDelegateInstance->setVisible(false);
            m_middleSelectionDelegateInstance->setParentItem(m_overlayContainer);

            m_bottomSelectionDelegateInstance = qobject_cast<QQuickItem*>(m_selectionDelegate->create(qmlContext(this)));
            m_bottomSelectionDelegateInstance->setVisible(false);
            m_bottomSelectionDelegateInstance->setParentItem(m_overlayContainer);
        }

        if (selection.top() == selection.bottom()) {
            QPointF start = charsToPixels(selection.topLeft());
            QPointF end = charsToPixels(selection.bottomRight());
            m_topSelectionDelegateInstance->setVisible(false);
            m_bottomSelectionDelegateInstance->setVisible(false);
            m_middleSelectionDelegateInstance->setVisible(true);
            m_middleSelectionDelegateInstance->setX(start.x());
            m_middleSelectionDelegateInstance->setY(start.y());
            m_middleSelectionDelegateInstance->setWidth(end.x() - start.x() + fontWidth());
            m_middleSelectionDelegateInstance->setHeight(end.y() - start.y() + fontHeight());
        } else {
            m_topSelectionDelegateInstance->setVisible(true);
            m_bottomSelectionDelegateInstance->setVisible(true);
            m_middleSelectionDelegateInstance->setVisible(true);

            QPointF start = charsToPixels(selection.topLeft());
            QPointF end = charsToPixels(QPoint(m_terminal.columns(), selection.top()));
            m_topSelectionDelegateInstance->setX(start.x());
            m_topSelectionDelegateInstance->setY(start.y());
            m_topSelectionDelegateInstance->setWidth(end.x() - start.x() + fontWidth());
            m_topSelectionDelegateInstance->setHeight(end.y() - start.y() + fontHeight());

            start = charsToPixels(QPoint(1, selection.top() + 1));
            end = charsToPixels(QPoint(m_terminal.columns(), selection.bottom() - 1));

            m_middleSelectionDelegateInstance->setX(start.x());
            m_middleSelectionDelegateInstance->setY(start.y());
            m_middleSelectionDelegateInstance->setWidth(end.x() - start.x() + fontWidth());
            m_middleSelectionDelegateInstance->setHeight(end.y() - start.y() + fontHeight());

            start = charsToPixels(QPoint(1, selection.bottom()));
            end = charsToPixels(selection.bottomRight());

            m_bottomSelectionDelegateInstance->setX(start.x());
            m_bottomSelectionDelegateInstance->setY(start.y());
            m_bottomSelectionDelegateInstance->setWidth(end.x() - start.x() + fontWidth());
            m_bottomSelectionDelegateInstance->setHeight(end.y() - start.y() + fontHeight());
        }
    } else if (m_topSelectionDelegateInstance) {
        m_topSelectionDelegateInstance->setVisible(false);
        m_bottomSelectionDelegateInstance->setVisible(false);
        m_middleSelectionDelegateInstance->setVisible(false);
    }
}

void TextRender::paintFromBuffer(const TerminalBuffer& buffer, int from, int to, qreal& y, int& yDelegateIndex)
{
    const int leftmargin = 2;
    int cutAfter = property("cutAfter").toInt() + iFontDescent;

    TermChar tmp = m_terminal.zeroChar;
    TermChar nextAttrib = m_terminal.zeroChar;
    TermChar currAttrib = m_terminal.zeroChar;
    qreal currentX = leftmargin;

    for (int i = from; i < to; i++, yDelegateIndex++) {
        y += iFontHeight;

        // ### if the background containers also had a container per row, we
        // could set the opacity there, rather than on each fragment.
        qreal opacity = 1.0;
        if (y >= cutAfter)
            opacity = 0.3;

        const auto& lineBuffer = buffer.at(i);
        int xcount = qMin(lineBuffer.size(), m_terminal.columns());

        // background for the current line
        currentX = leftmargin;
        qreal fragWidth = 0;
        for (int j = 0; j < xcount; j++) {
            fragWidth += iFontWidth;
            if (j == 0) {
                tmp = lineBuffer.at(j);
                currAttrib = tmp;
                nextAttrib = tmp;
            } else if (j < xcount - 1) {
                nextAttrib = lineBuffer.at(j + 1);
            }

            if (currAttrib.attrib != nextAttrib.attrib || currAttrib.bgColor != nextAttrib.bgColor || currAttrib.fgColor != nextAttrib.fgColor || j == xcount - 1) {
                QQuickItem* backgroundRectangle = fetchFreeCell();
                drawBgFragment(backgroundRectangle, currentX, y - iFontHeight + iFontDescent, std::ceil(fragWidth), currAttrib);
                backgroundRectangle->setOpacity(opacity);
                currentX += fragWidth;
                fragWidth = 0;
                currAttrib.attrib = nextAttrib.attrib;
                currAttrib.bgColor = nextAttrib.bgColor;
                currAttrib.fgColor = nextAttrib.fgColor;
            }
        }

        // text for the current line
        QString line;
        currentX = leftmargin;
        for (int j = 0; j < xcount; j++) {
            tmp = lineBuffer.at(j);
            line += tmp.c;
            if (j == 0) {
                currAttrib = tmp;
                nextAttrib = tmp;
            } else if (j < xcount - 1) {
                nextAttrib = lineBuffer.at(j + 1);
            }

            if (currAttrib.attrib != nextAttrib.attrib || currAttrib.bgColor != nextAttrib.bgColor || currAttrib.fgColor != nextAttrib.fgColor || j == xcount - 1) {
                QQuickItem* foregroundText = fetchFreeCellContent();
                drawTextFragment(foregroundText, currentX, y - iFontHeight + iFontDescent, line, currAttrib);
                foregroundText->setOpacity(opacity);
                currentX += iFontWidth * line.length();
                line.clear();
                currAttrib.attrib = nextAttrib.attrib;
                currAttrib.bgColor = nextAttrib.bgColor;
                currAttrib.fgColor = nextAttrib.fgColor;
            }
        }
    }
}

void TextRender::drawBgFragment(QQuickItem* cellDelegate, qreal x, qreal y, int width, TermChar style)
{
    if (style.attrib & TermChar::NegativeAttribute) {
        QRgb c = style.fgColor;
        style.fgColor = style.bgColor;
        style.bgColor = c;
    }

    QColor qtColor;

    if (m_terminal.inverseVideoMode() && style.bgColor == Parser::fetchDefaultBgColor()) {
        qtColor = Parser::fetchDefaultFgColor();
    } else {
        qtColor = style.bgColor;
    }

    cellDelegate->setX(x);
    cellDelegate->setY(y);
    cellDelegate->setWidth(width);
    cellDelegate->setHeight(iFontHeight);
    cellDelegate->setProperty("color", qtColor);
    cellDelegate->setVisible(true);
}

void TextRender::drawTextFragment(QQuickItem* cellContentsDelegate, qreal x, qreal y, QString text, TermChar style)
{
    if (style.attrib & TermChar::NegativeAttribute) {
        QRgb c = style.fgColor;
        style.fgColor = style.bgColor;
        style.bgColor = c;
    }
    if (style.attrib & TermChar::BoldAttribute) {
        iFont.setBold(true);
    } else if (iFont.bold()) {
        iFont.setBold(false);
    }
    if (style.attrib & TermChar::UnderlineAttribute) {
        iFont.setUnderline(true);
    } else if (iFont.underline()) {
        iFont.setUnderline(false);
    }
    if (style.attrib & TermChar::ItalicAttribute) {
        iFont.setItalic(true);
    } else if (iFont.italic()) {
        iFont.setItalic(false);
    }

    QColor qtColor;

    if (m_terminal.inverseVideoMode() && style.fgColor == Parser::fetchDefaultFgColor()) {
        qtColor = Parser::fetchDefaultBgColor();
    } else {
        qtColor = style.fgColor;
    }

    cellContentsDelegate->setX(x);
    cellContentsDelegate->setY(y);
    cellContentsDelegate->setHeight(iFontHeight);
    cellContentsDelegate->setProperty("color", qtColor);
    cellContentsDelegate->setProperty("text", text);
    cellContentsDelegate->setProperty("font", iFont);

    if (style.attrib & TermChar::BlinkAttribute) {
        cellContentsDelegate->setProperty("blinking", true);
    } else {
        cellContentsDelegate->setProperty("blinking", false);
    }

    cellContentsDelegate->setVisible(true);
}

void TextRender::redraw()
{
    TRACE();
    if (!m_dispatch_timer) {
        m_dispatch_timer = new QTimer(this);
        m_dispatch_timer->setSingleShot(true);
        m_dispatch_timer->setInterval(50);
        connect(m_dispatch_timer, &QTimer::timeout, this, &TextRender::polish);
    }
    if (!m_dispatch_timer->isActive()) m_dispatch_timer->start();
}

void TextRender::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !iAllowGestures || m_dragMode == DragOff) {
        event->ignore();
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setMousePressed(event->localPos().x(), event->localPos().y());
#else
    setMousePressed(event->position().x(), event->position().y());
#endif
}

void TextRender::setMousePressed(int x, int y)
{
    m_activeClick = true;
    m_touchMoved = false;
    dragOrigin.setX(x);
    dragOrigin.setY(y);

    if (!m_mouse_timer) {
        m_mouse_timer = new QTimer(this);
        m_mouse_timer->setSingleShot(true);
        m_mouse_timer->setInterval(QGuiApplication::styleHints()->mousePressAndHoldInterval());
        m_mouse_timer->callOnTimeout([this]() {
            m_activeClick = false;
            m_touchMoved = false;
            emit mouseLongPressed();
        });
    }
    m_mouse_timer->start();

    if (!hasActiveFocus())
        forceActiveFocus(Qt::MouseFocusReason);
}

void TextRender::mouseMoveEvent(QMouseEvent* event)
{
    if (!iAllowGestures || m_dragMode == DragOff || !m_activeClick) {
        event->ignore();
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setMouseMoved(event->localPos().x(), event->localPos().y());
#else
    setMouseMoved(event->position().x(), event->position().y());
#endif
}

void TextRender::setMouseMoved(int x, int y)
{
    if ((QPointF(x, y) - dragOrigin).manhattanLength() < iFontWidth) // short distance, assumed as jitter
        return;

    if (m_mouse_timer) m_mouse_timer->stop();
    m_touchMoved = true;

    if (m_dragMode == DragScroll) {
        dragOrigin = scrollBackBuffer(QPointF(x, y), dragOrigin);
    } else if (m_dragMode == DragSelect) {
        selectionHelper(QPointF(x, y), true);
    }
}

void TextRender::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !iAllowGestures || m_dragMode == DragOff || !m_activeClick) {
        event->ignore();
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setMouseReleased(event->localPos().x(), event->localPos().y());
#else
    setMouseReleased(event->position().x(), event->position().y());
#endif
}

void TextRender::setMouseReleased(int x, int y)
{
    static const int reqDragLength = 140;

    if (m_mouse_timer) m_mouse_timer->stop();

    if (m_touchMoved) {
        m_touchMoved = false;
        if (m_dragMode == DragGestures) {
            int xdist = qAbs(dragOrigin.x() - x);
            int ydist = qAbs(dragOrigin.y() - y);
            if (dragOrigin.x() < x - reqDragLength && xdist > ydist * 2) emit panLeft();
            else if (dragOrigin.x() > x + reqDragLength && xdist > ydist * 2) emit panRight();
            else if (dragOrigin.y() > y + reqDragLength && ydist > xdist * 2) emit panDown();
            else if (dragOrigin.y() < y - reqDragLength && ydist > xdist * 2) emit panUp();
        } else if (m_dragMode == DragScroll) {
            scrollBackBuffer(QPointF(x, y), dragOrigin);
        } else if (m_dragMode == DragSelect) {
            selectionHelper(QPointF(x, y), false);
        }
    } else if (x == dragOrigin.x() && y == dragOrigin.y()) {
        if (m_dragMode == DragSelect) m_terminal.clearSelection();
        if (hasActiveFocus()) emit mouseClicked();
    }
}

void TextRender::touchEvent(QTouchEvent *event)
{
    if (!iAllowGestures || m_dragMode == DragOff) {
        event->ignore();
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto points = event->touchPoints();
#else
    const auto points = event->points();
#endif
    if (points.size() != 1) {
        event->ignore();
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    int x = points.first().pos().x();
    int y = points.first().pos().y();
#else
    int x = points.first().position().x();
    int y = points.first().position().y();
#endif
    auto states = event->touchPointStates();
    if (states & Qt::TouchPointPressed) {
        setMousePressed(x, y);
        return;
    }
    if (!m_activeClick) {
        event->ignore();
        return;
    }
    if (states & Qt::TouchPointMoved) setMouseMoved(x, y);
    else if (states & Qt::TouchPointReleased) setMouseReleased(x, y);
    //XXX else if (states & Qt::TouchPointStationary) { ? }
}

void TextRender::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Back) {
        event->ignore();
        return;
    }
    if (m_terminal.keyPress(event->key(), event->modifiers(), event->text()))
        emit keyPressed();
}

void TextRender::inputMethodEvent(QInputMethodEvent *event)
{
    if (event->commitString().isEmpty()) {
        event->ignore();
        return;
    }
    if (m_terminal.keyPress(event->commitString().front().unicode(), m_modifiers, QString()))
        emit keyPressed();
}

void TextRender::extKeyPressed(int key)
{
    if (m_terminal.keyPress(key, m_modifiers, QString()))
        emit keyPressed();
}

void TextRender::extKeyToggled(int key, bool checked)
{
    TRACE_ARG(key << checked);
    if (checked) {
        if (key == Qt::Key_Control) m_modifiers |= Qt::ControlModifier;
        else if (key == Qt::Key_Alt) m_modifiers |= Qt::AltModifier;
    } else {
        if (key == Qt::Key_Control) m_modifiers &= ~Qt::ControlModifier;
        else if (key == Qt::Key_Alt) m_modifiers &= ~Qt::AltModifier;
    }
}

QVariant TextRender::inputMethodQuery(Qt::InputMethodQuery query) const
{
    auto var = QQuickItem::inputMethodQuery(query);
    if (!(query & Qt::ImHints)) return var;
    int bits = var.isNull() ? 0 : var.toUInt();
    bits |= (Qt::ImhSensitiveData | Qt::ImhNoAutoUppercase | Qt::ImhPreferLowercase |
             Qt::ImhNoPredictiveText | Qt::ImhPreferLatin | Qt::ImhMultiLine | Qt::ImhNoEditMenu);
    return bits;
}

void TextRender::selectionHelper(QPointF scenePos, bool selectionOngoing)
{
    int yCorr = fontDescent();

    QPoint start(qRound((dragOrigin.x() + 2) / fontWidth()),
        qRound((dragOrigin.y() + yCorr) / fontHeight()));
    QPoint end(qRound((scenePos.x() + 2) / fontWidth()),
        qRound((scenePos.y() + yCorr) / fontHeight()));

    if (start != end) {
        m_terminal.setSelection(start, end, selectionOngoing);
    }
}

void TextRender::handleScrollBack(bool reset)
{
    if (reset) {
        setShowBufferScrollIndicator(false);
    } else {
        setShowBufferScrollIndicator(m_terminal.backBufferScrollPos() != 0);
    }
    redraw();
}

QPointF TextRender::cursorPixelPos()
{
    return charsToPixels(m_terminal.cursorPos());
}

QPointF TextRender::charsToPixels(QPoint pos)
{
    qreal x = 2;                     // left margin
    x += iFontWidth * (pos.x() - 1); // 0 indexed, so -1

    qreal y = iFontHeight * (pos.y() - 1) + iFontDescent + 1;

    return QPointF(x, y);
}

QSizeF TextRender::cellSize()
{
    return QSizeF(iFontWidth, iFontHeight);
}

bool TextRender::allowGestures()
{
    return iAllowGestures;
}

void TextRender::setAllowGestures(bool allow)
{
    if (iAllowGestures != allow) {
        iAllowGestures = allow;
        emit allowGesturesChanged();
    }

    if (!allow) {
        m_activeClick = false;
    }
}

QQmlComponent* TextRender::cellDelegate() const
{
    return m_cellDelegate;
}

void TextRender::setCellDelegate(QQmlComponent* component)
{
    if (m_cellDelegate == component)
        return;

    qDeleteAll(m_cells);
    qDeleteAll(m_freeCells);
    m_cells.clear();
    m_freeCells.clear();
    m_cellDelegate = component;
    emit cellDelegateChanged();
    polish();
}

QQmlComponent* TextRender::cellContentsDelegate() const
{
    return m_cellContentsDelegate;
}

void TextRender::setCellContentsDelegate(QQmlComponent* component)
{
    if (m_cellContentsDelegate == component)
        return;

    qDeleteAll(m_cellsContent);
    qDeleteAll(m_freeCellsContent);
    m_cellsContent.clear();
    m_freeCellsContent.clear();
    m_cellContentsDelegate = component;
    emit cellContentsDelegateChanged();
    polish();
}

QQmlComponent* TextRender::cursorDelegate() const
{
    return m_cursorDelegate;
}

void TextRender::setCursorDelegate(QQmlComponent* component)
{
    if (m_cursorDelegate == component)
        return;

    delete m_cursorDelegateInstance;
    m_cursorDelegateInstance = 0;
    m_cursorDelegate = component;

    emit cursorDelegateChanged();
    polish();
}

QQmlComponent* TextRender::selectionDelegate() const
{
    return m_selectionDelegate;
}

void TextRender::setSelectionDelegate(QQmlComponent* component)
{
    if (m_selectionDelegate == component)
        return;

    delete m_topSelectionDelegateInstance;
    delete m_middleSelectionDelegateInstance;
    delete m_bottomSelectionDelegateInstance;
    m_topSelectionDelegateInstance = 0;
    m_middleSelectionDelegateInstance = 0;
    m_bottomSelectionDelegateInstance = 0;

    m_selectionDelegate = component;

    emit selectionDelegateChanged();
    polish();
}

int TextRender::contentHeight() const
{
    int lines = m_terminal.useAltScreenBuffer() ? m_terminal.buffer().size()
                : m_terminal.buffer().size() + m_terminal.backBuffer().size();
    return qCeil(lines * iFontHeight);
}

int TextRender::visibleHeight() const
{
    return qCeil(m_terminal.buffer().size() * iFontHeight);
}

int TextRender::contentY() const
{
    if (m_terminal.useAltScreenBuffer()) return 0;
    int lines = m_terminal.backBuffer().size() - m_terminal.backBufferScrollPos();
    return qFloor(lines * iFontHeight);
}

void TextRender::setContentY(int ypos)
{
    int lines = qFloor(ypos / iFontHeight);
    if (lines != m_terminal.backBufferScrollPos()) {
        if (lines > m_terminal.backBufferScrollPos())
            m_terminal.scrollBackBufferBack(lines - m_terminal.backBufferScrollPos());
        else m_terminal.scrollBackBufferFwd(m_terminal.backBufferScrollPos() - lines);
    }
}

QPointF TextRender::scrollBackBuffer(QPointF now, QPointF last)
{
    int xdist = qAbs(now.x() - last.x());
    int ydist = qAbs(now.y() - last.y());
    int fontSize = fontPointSize();
    int lines = ydist / fontSize;

    if (lines > 0 && now.y() < last.y() && xdist < ydist * 2) {
        m_terminal.scrollBackBufferFwd(lines);
        last = QPointF(now.x(), last.y() - lines * fontSize);
    } else if (lines > 0 && now.y() > last.y() && xdist < ydist * 2) {
        m_terminal.scrollBackBufferBack(lines);
        last = QPointF(now.x(), last.y() + lines * fontSize);
    }
    return last;
}

QObject *TextRender::session() const
{
    return ssh_session ? ssh_session.data() : nullptr;
}

void TextRender::setSession(QObject *obj)
{
    TRACE_ARG(obj);
    if (!obj) {
        if (ssh_session) {
            ssh_session->cancel();
            ssh_session->deleteLater();
            ssh_session.clear();
        }
        return;
    }
    if (ssh_session) {
        qWarning() << Q_FUNC_INFO << "SshSession already set";
        return;
    }
    auto ss = qobject_cast<SshSession*>(obj);
    if (!ss) {
        qWarning() << Q_FUNC_INFO << "Bad Object type, SystemProcess or SshSession expected";
        return;
    }
    if (!ss->isReady()) {
        qWarning() << Q_FUNC_INFO << "Ssh session must be ready!";
        return;
    }
    ssh_session = ss;
    emit sessionChanged();
}
