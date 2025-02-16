#ifndef WINDOWSTATE_H
#define WINDOWSTATE_H

#include <QObject>

class WindowState : public QObject
{
    Q_OBJECT
public:
    explicit WindowState(QObject *parent = nullptr);

    enum State { // bit-mapped
        NoState    = Qt::WindowNoState,    // 0x0: no state set (in normal state)
        Minimized  = Qt::WindowMinimized,  // 0x1: minimized (i.e. iconified)
        Maximized  = Qt::WindowMaximized,  // 0x2: maximized with a frame around it
        FullScreen = Qt::WindowFullScreen, // 0x4: fills entire screen without frame around
        Active     = Qt::WindowActive      // 0x8: active window, i.e. has keyboard focus
    };
    Q_ENUM(State)

protected:
     bool eventFilter(QObject *obj, QEvent *event) override;

signals:
     void stateChanged(int state);
};

#endif // WINDOWSTATE_H
