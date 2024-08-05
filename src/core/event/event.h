#pragma once

#include <string>
#include <sstream>
#include <iostream>

enum class EventType
{
    None = 0,
    WindowClose,
    WindowResize,
    WindowMoved,
    KeyPressed,
    KeyReleased,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

enum EventCategory
{
    None = 0,
    EventCategoryApplication = (1 << 0),
    EventCategoryInput = (1 << 1),
    EventCategoryKeyboard = (1 << 2),
    EventCategoryMouse = (1 << 3),
    EventCategoryMouseButton = (1 << 4),
};

#define EVENT_CLASS_TYPE(type)              \
static EventType GetStaticType()        \
{                                       \
    return EventType::type;             \
}                                       \
EventType GetEventType() const override \
{                                       \
    return GetStaticType();             \
}                                       \
const char* GetName() const override    \
{                                       \
    return #type;                       \
}
#define EVENT_CATEGORY(category)          \
int GetCategoryFlags() const override \
{                                     \
    return category;                  \
}

class Event
{
public:
    virtual ~Event() = default;

    bool Handled = false;

    virtual EventType	GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int			GetCategoryFlags() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool InCategory(EventCategory category) const { return GetCategoryFlags() & category; }
};

class EventDispatcher
{
public:
    explicit EventDispatcher(Event& event)
        : m_Event(event) {}

    template <typename T, typename F>
    bool Dispatch(const F& func)
    {
        if (T::GetStaticType() == m_Event.GetEventType())
        {
            m_Event.Handled |= func(static_cast<T&>(m_Event));
            return true;
        }

        return false;
    }

private:
    Event& m_Event;
};

inline std::ostream& operator<<(std::ostream& stream, const Event& e)
{
    return stream << e.ToString();
}
