#include <QDebug>

#include "DesktopAction.h"

class DesktopActionData : public QSharedData
{
public:
    DesktopActionData()
        : type(DesktopAction::ActionNone)
        , button(0)
        , scanCode(0)
        , virtualKey(0)
        , down(false)
        , posX(0)
        , posY(0)
        , deltaX(0)
        , deltaY(0)
    {}
    DesktopActionData(const DesktopActionData &other)
        : QSharedData(other)
        , type(other.type)
        , button(other.button)
        , scanCode(other.scanCode)
        , virtualKey(other.virtualKey)
        , down(other.down)
        , posX(other.posX)
        , posY(other.posY)
        , deltaX(other.deltaX)
        , deltaY(other.deltaY)
    {}
    ~DesktopActionData() {}

    DesktopAction::ActionType type;
    int     button;
    quint32 scanCode;
    quint32 virtualKey;
    bool    down;
    int     posX;
    int     posY;
    bool    move;
    int     deltaX;
    int     deltaY;
};

DesktopAction::DesktopAction()
    : d(new DesktopActionData)
{
}

DesktopAction::DesktopAction(int key, quint32 scanCode, quint32 virtualKey, bool down)
    : d(new DesktopActionData)
{
    d->type = ActionKey;
    d->button = key;
    d->scanCode = scanCode;
    d->virtualKey = virtualKey;
    d->down = down;
}

DesktopAction::DesktopAction(int posX, int posY, int button, bool move, bool down)
    : d(new DesktopActionData)
{
    d->type = ActionMouse;
    d->posX = posX;
    d->posY = posY;
    d->button = button;
    d->move = move;
    d->down = down;
}

DesktopAction::DesktopAction(int posX, int posY, int deltaX, int deltaY)
    : d(new DesktopActionData)
{
    d->type = ActionWheel;
    d->posX = posX;
    d->posY = posY;
    d->deltaX = deltaX;
    d->deltaY = deltaY;
}

DesktopAction::DesktopAction(const DesktopAction &other)
    : d(other.d)
{
}

DesktopAction &DesktopAction::operator=(const DesktopAction &other)
{
    if (this != &other) d.operator=(other.d);
    return *this;
}

DesktopAction::~DesktopAction()
{
}

bool DesktopAction::operator==(const DesktopAction &other) const
{
    return (d->type == other.d->type &&
            d->button == other.d->button &&
            d->scanCode == other.d->scanCode &&
            d->virtualKey == other.d->virtualKey &&
            d->down == other.d->down &&
            d->posX == other.d->posX &&
            d->posY == other.d->posY &&
            d->move == other.d->move &&
            d->deltaX == other.d->deltaX &&
            d->deltaY == other.d->deltaY);
}

bool DesktopAction::operator!=(const DesktopAction &other) const
{
    return (d->type != other.d->type ||
            d->button != other.d->button ||
            d->scanCode != other.d->scanCode ||
            d->virtualKey != other.d->virtualKey ||
            d->down != other.d->down ||
            d->posX != other.d->posX ||
            d->posY != other.d->posY ||
            d->move != other.d->move ||
            d->deltaX != other.d->deltaX ||
            d->deltaY != other.d->deltaY);
}

bool DesktopAction::isValid() const
{
    switch (d->type) {
    case ActionKey:   return (d->button || d->scanCode || d->virtualKey);
    case ActionMouse: return (d->posX || d->posY || d->button || d->move || d->down);
    case ActionWheel: return (d->posX || d->posY || d->deltaX || d->deltaY);
    case ActionNone:  break;
    }
    return false;
}

DesktopAction::ActionType DesktopAction::type() const
{
    return d->type;
}

int DesktopAction::key() const
{
    return d->button;
}

quint32 DesktopAction::scanCode() const
{
    return d->scanCode;
}

quint32 DesktopAction::virtualKey() const
{
    return d->virtualKey;
}

int DesktopAction::posX() const
{
    return d->posX;
}

int DesktopAction::posY() const
{
    return d->posY;
}

bool DesktopAction::move() const
{
    return d->move;
}

int DesktopAction::button() const
{
    return d->button;
}

bool DesktopAction::down() const
{
    return d->down;
}

int DesktopAction::deltaX() const
{
    return d->deltaX;
}

int DesktopAction::deltaY() const
{
    return d->deltaY;
}

void DesktopAction::dump(QDebug &dbg) const
{
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg << "type"       << d->type
        << "button"     << d->button
        << "scanCode"   << d->scanCode
        << "virtualKey" << d->virtualKey
        << "down"       << d->down
        << "posX"       << d->posX
        << "posY"       << d->posY
        << "move"       << d->move
        << "deltaX"     << d->deltaX
        << "deltaY"     << d->deltaY;
}
