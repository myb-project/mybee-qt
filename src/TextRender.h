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

#ifndef TEXTRENDER_H
#define TEXTRENDER_H

#include <QQuickItem>
#include <QPointer>
#include <QWeakPointer>

#include "Terminal.h"

class QTimer;
class SshSession;
class SshChannelShell;

class TextRender : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(DragMode dragMode READ dragMode WRITE setDragMode NOTIFY dragModeChanged)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QSizeF cellSize READ cellSize NOTIFY cellSizeChanged)
    Q_PROPERTY(bool showBufferScrollIndicator READ showBufferScrollIndicator WRITE setShowBufferScrollIndicator NOTIFY showBufferScrollIndicatorChanged)
    Q_PROPERTY(bool allowGestures READ allowGestures WRITE setAllowGestures NOTIFY allowGesturesChanged)
    Q_PROPERTY(QQmlComponent* cellDelegate READ cellDelegate WRITE setCellDelegate NOTIFY cellDelegateChanged)
    Q_PROPERTY(QQmlComponent* cellContentsDelegate READ cellContentsDelegate WRITE setCellContentsDelegate NOTIFY cellContentsDelegateChanged)
    Q_PROPERTY(QQmlComponent* cursorDelegate READ cursorDelegate WRITE setCursorDelegate NOTIFY cursorDelegateChanged)
    Q_PROPERTY(QQmlComponent* selectionDelegate READ selectionDelegate WRITE setSelectionDelegate NOTIFY selectionDelegateChanged)
    Q_PROPERTY(int contentHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(int visibleHeight READ visibleHeight NOTIFY visibleHeightChanged)
    Q_PROPERTY(int contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(QSize terminalSize READ terminalSize NOTIFY terminalSizeChanged)
    Q_PROPERTY(bool selected READ selected NOTIFY selectedChanged FINAL)
    Q_PROPERTY(QObject* session READ session WRITE setSession NOTIFY sessionChanged FINAL)

public:
    explicit TextRender(QQuickItem *parent = nullptr);
    ~TextRender();

    Q_INVOKABLE QStringList printableLinesFromCursor(int lines) const;
    Q_INVOKABLE void putString(const QString &str);
    Q_INVOKABLE QStringList grabURLsFromBuffer() const;

    bool selected() const;
    Q_INVOKABLE bool copy();
    Q_INVOKABLE bool paste();

    Q_INVOKABLE void deselect();
    Q_INVOKABLE QString selectedText() const;

    int contentHeight() const;
    int visibleHeight() const;
    int contentY() const;
    void setContentY(int ypos);

    QString title() const;

    enum DragMode
    {
        DragOff,
        DragGestures,
        DragScroll,
        DragSelect
    };
    Q_ENUM(DragMode)

    DragMode dragMode() const;
    void setDragMode(DragMode dragMode);

    QQuickItem* contentItem() const { return m_contentItem; }
    void setContentItem(QQuickItem* contentItem);

    QSize terminalSize() const;
    QFont font() const;
    void setFont(const QFont& font);
    bool showBufferScrollIndicator() const;
    void setShowBufferScrollIndicator(bool on);

    Q_INVOKABLE QPointF cursorPixelPos();
    QSizeF cellSize();

    bool allowGestures();
    void setAllowGestures(bool allow);

    QQmlComponent* cellDelegate() const;
    void setCellDelegate(QQmlComponent* delegate);
    QQmlComponent* cellContentsDelegate() const;
    void setCellContentsDelegate(QQmlComponent* delegate);
    QQmlComponent* cursorDelegate() const;
    void setCursorDelegate(QQmlComponent* delegate);
    QQmlComponent* selectionDelegate() const;
    void setSelectionDelegate(QQmlComponent* delegate);

    QObject *session() const;
    void setSession(QObject *obj);

signals:
    void contentItemChanged();
    void fontChanged();
    void cellSizeChanged();
    void showBufferScrollIndicatorChanged();
    void allowGesturesChanged();
    void cellDelegateChanged();
    void cellContentsDelegateChanged();
    void cursorDelegateChanged();
    void selectionDelegateChanged();
    void visualBell();
    void titleChanged();
    void dragModeChanged();
    void contentHeightChanged();
    void visibleHeightChanged();
    void contentYChanged();
    void terminalSizeChanged();
    void selectedChanged();
    void displayBufferChanged();
    void panLeft();
    void panRight();
    void panUp();
    void panDown();
    void hangupReceived();
    void mouseClicked();
    void mouseLongPressed();
    void keyPressed();

    void sessionChanged();
    void terminalReady();

public slots:
    void redraw();
    void extKeyPressed(int key);
    void extKeyToggled(int key, bool checked);

protected:
    // Reimplemented from QQuickItem
    void updatePolish() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void componentComplete() override;
    void touchEvent(QTouchEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent *event) override;

private slots:
    void handleScrollBack(bool reset);
    void handleTitleChanged(const QString& title);

private:
    Q_DISABLE_COPY(TextRender)

    enum PanGesture
    {
        PanNone,
        PanLeft,
        PanRight,
        PanUp,
        PanDown
    };

    void drawBgFragment(QQuickItem* cellContentsDelegate, qreal x, qreal y, int width, TermChar style);
    void drawTextFragment(QQuickItem* cellContentsDelegate, qreal x, qreal y, QString text, TermChar style);
    void paintFromBuffer(const TerminalBuffer& buffer, int from, int to, qreal& y, int& yDelegateIndex);
    QPointF charsToPixels(QPoint pos);
    void selectionHelper(QPointF scenePos, bool selectionOngoing);
    void setFontMetrics();
    void setMousePressed(int x, int y);
    void setMouseMoved(int x, int y);
    void setMouseReleased(int x, int y);

    qreal fontWidth() const { return iFontWidth; }
    qreal fontHeight() const { return iFontHeight; }
    qreal fontDescent() const { return iFontDescent; }
    int fontPointSize() const { return iFont.pointSize(); }

    /**
     * Scroll the back buffer on drag.
     *
     * @param now The current position
     * @param last The last position (or start position)
     * @return The new value for last (modified by any consumed offset)
     **/
    QPointF scrollBackBuffer(QPointF now, QPointF last);

    QQuickItem* fetchFreeCell();
    QQuickItem* fetchFreeCellContent();

    QPointF dragOrigin;
    bool m_activeClick;
    bool m_touchMoved;
    int m_modifiers;

    QFont iFont;
    qreal iFontWidth;
    qreal iFontHeight;
    qreal iFontDescent;
    bool iShowBufferScrollIndicator;
    bool iAllowGestures;
    bool last_selected;

    QQuickItem* m_contentItem;
    QQuickItem* m_backgroundContainer;
    QQuickItem* m_textContainer;
    QQuickItem* m_overlayContainer;
    QQmlComponent* m_cellDelegate;
    QVector<QQuickItem*> m_cells;
    QVector<QQuickItem*> m_freeCells;
    QQmlComponent* m_cellContentsDelegate;
    QVector<QQuickItem*> m_cellsContent;
    QVector<QQuickItem*> m_freeCellsContent;
    QQmlComponent* m_cursorDelegate;
    QQuickItem* m_cursorDelegateInstance;
    QQmlComponent* m_selectionDelegate;
    QQuickItem* m_topSelectionDelegateInstance;
    QQuickItem* m_middleSelectionDelegateInstance;
    QQuickItem* m_bottomSelectionDelegateInstance;
    DragMode m_dragMode;
    QString m_title;
    QTimer *m_dispatch_timer;
    QTimer *m_mouse_timer;
    Terminal m_terminal;

    QPointer<SshSession> ssh_session;
    QWeakPointer<SshChannelShell> ssh_shell;
};

#endif // TEXTRENDER_H
