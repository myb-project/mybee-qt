#include <QGuiApplication>
#include <QWindow>

#include "WindowState.h"
//#include "TextRender.h"

WindowState::WindowState(QObject *parent)
    : QObject(parent)
{
    auto app = QGuiApplication::instance();
    Q_ASSERT(app);
    app->installEventFilter(this);
}

bool WindowState::eventFilter(QObject *obj, QEvent *event)
{
    /*auto term = dynamic_cast<TextRender *>(obj);
    if (term) qDebug() << Q_FUNC_INFO << event->type();*/

    if (event->type() == QEvent::WindowStateChange) {
        auto win = dynamic_cast<QWindow *>(obj);
        if (win) {
            emit stateChanged(win->windowStates());
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}
