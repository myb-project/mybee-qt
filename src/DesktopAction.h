#ifndef DESKTOPACTION_H
#define DESKTOPACTION_H

#include <QSharedDataPointer>
#include <QMetaType>

class DesktopActionData;

class DesktopAction
{
public:
    DesktopAction();
    DesktopAction(int key, quint32 scanCode, quint32 virtualKey, bool down);
    DesktopAction(int posX, int posY, int button, bool move, bool down);
    DesktopAction(int posX, int posY, int deltaX, int deltaY);
    DesktopAction(const DesktopAction &other);
    DesktopAction &operator=(const DesktopAction &other);
    ~DesktopAction();

    bool operator==(const DesktopAction &other) const;
    bool operator!=(const DesktopAction &other) const;

    enum ActionType { ActionNone, ActionKey, ActionMouse, ActionWheel };

    bool isValid() const;
    ActionType type() const;

    int key() const;            // key unicode?
    quint32 scanCode() const;   // keyboard actions
    quint32 virtualKey() const; // keyboard keysym
    bool down() const;          // pressed or released

    int posX() const;   // mouse position X
    int posY() const;   // mouse position Y
    int button() const; // mouse button
    bool move() const;  // mouse moving or stay in place

    int deltaX() const; // wheel angle X
    int deltaY() const; // wheel angle Y

    friend inline QDebug& operator<<(QDebug &dbg, const DesktopAction &from) {
        from.dump(dbg); return dbg; }

private:
    void dump(QDebug &dbg) const;

    QSharedDataPointer<DesktopActionData> d;
};

Q_DECLARE_TYPEINFO(DesktopAction, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(DesktopAction)

#endif // DESKTOPACTION_H
