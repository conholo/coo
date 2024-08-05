#pragma once

#include "core/event/event.h"

class WindowClosedEvent : public Event
{
public:
    EVENT_CLASS_TYPE(WindowClose);
    EVENT_CATEGORY(EventCategoryApplication);
};

class WindowResizedEvent : public Event
{
public:
    WindowResizedEvent(int width, int height)
        : m_Width(width), m_Height(height) {}

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizedEvent: "
           << "(" << m_Width << "," << m_Height << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowResize);
    EVENT_CATEGORY(EventCategoryApplication);

private:
    int m_Width, m_Height;
};

class WindowMovedEvent : public Event
{
public:
    WindowMovedEvent(int xPosition, int yPosition)
        : m_XPosition(xPosition), m_YPosition(yPosition) {}

    int GetXPosition() const { return m_XPosition; }
    int GetYPosition() const { return m_YPosition; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowMovedEvent: "
           << "(" << m_XPosition << "," << m_YPosition << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowMoved);
    EVENT_CATEGORY(EventCategoryApplication);

private:
    int m_XPosition, m_YPosition;
};
