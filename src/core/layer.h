#pragma once

#include "event/event.h"
#include <string>

class Layer
{
public:

	Layer(const std::string& name = "Layer")
		:m_Name(name) { }
	virtual ~Layer() = default;

	virtual void OnAttach() = 0;
	virtual void OnDetach() = 0;
	virtual void OnUpdate(float deltaTime) { }
	virtual void OnEvent(Event& event) { }
	virtual void OnUIRender() { }

	const std::string& GetName() const { return m_Name; }

private:
	std::string m_Name;
};
