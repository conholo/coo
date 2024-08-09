#pragma once

#include "core/event/event.h"

class KeyEvent : public Event
{
public:
    int GetKeyCode() const { return m_KeyCode; }

    EVENT_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

protected:
    KeyEvent(int keyCode)
        : m_KeyCode(keyCode) {}

    int m_KeyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int keyCode, int repeatCount)
        : KeyEvent(keyCode), m_RepeatCount(repeatCount) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: "
           << "(" << m_KeyCode << " - " << m_RepeatCount << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed);

private:
    int m_RepeatCount;
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keyCode)
        : KeyEvent(keyCode) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: "
           << "(" << m_KeyCode << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased);
};
