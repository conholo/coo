#pragma once
#include <vulkan/vulkan.h>
#include <memory>

struct FrameInfo;
class IRenderable
{
public:
    virtual void Render(FrameInfo& frameInfo) = 0;
};