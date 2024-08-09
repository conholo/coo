#pragma once
#include "core/frame_info.h"

class IRenderer
{
public:
	virtual ~IRenderer() = default;
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual void Render(FrameInfo& frameInfo) = 0;
	virtual void Resize(uint32_t width, uint32_t height) = 0;
	virtual void RegisterGameObject(GameObject& gameObject) = 0;
};

