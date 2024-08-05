#pragma once

#include "core/event/event.h"

class MouseButtonEvent : public Event
{
public:
    int GetButton() const { return m_Button; }
    double GetMouseX() const { return m_XPosition; }
    double GetMouseY() const { return m_YPosition; }

    EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton);

protected:
    MouseButtonEvent(int button, double xPosition, double yPosition)
        : m_Button(button), m_XPosition(xPosition), m_YPosition(yPosition) {}

    int m_Button;
    double m_XPosition, m_YPosition;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(int button, double xPosition, double yPosition)
        : MouseButtonEvent(button, xPosition, yPosition) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseButtonPressedEvent: " << m_Button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonPressed);
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(int button, double xPosition, double yPosition)
        : MouseButtonEvent(button, xPosition, yPosition) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseButtonReleasedEvent: " << m_Button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonReleased);
};

class MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_XOffset(xOffset), m_YOffset(yOffset) {}

    float GetXOffset() const { return m_XOffset; }
    float GetYOffset() const { return m_YOffset; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseScrollEvent: "
           << "(" << m_XOffset << "," << m_YOffset << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseScrolled);
    EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput);

private:
    float m_XOffset, m_YOffset;
};

class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(float xPosition, float yPosition)
        : m_XPosition(xPosition), m_YPosition(yPosition) {}

    float GetXPosition() const { return m_XPosition; }
    float GetYPosition() const { return m_YPosition; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseMovedEvent: "
           << "(" << m_XPosition << "," << m_YPosition << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(MouseMoved);
    EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput);

private:
    float m_XPosition, m_YPosition;
};
