#pragma once
#include "core/frame_info.h"
#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan_framebuffer.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_render_pass.h"

class IRenderer
{
public:
	virtual ~IRenderer() = default;
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual void Render(FrameInfo& frameInfo) = 0;
	virtual void OnEvent(Event& event) = 0;
	virtual void Resize(uint32_t width, uint32_t height) = 0;
	virtual void RegisterGameObject(GameObject& gameObject) = 0;
	virtual VulkanRenderPass& GetRenderFinishedRenderPass() = 0;
	virtual VkSemaphore GetRendererFinishedSemaphore(uint32_t frameIndex) = 0;
	virtual VulkanFramebuffer& GetRenderFinishedFramebuffer(uint32_t frameIndex) = 0;
};

class IRenderPass
{
public:
	virtual ~IRenderPass() = default;

	virtual void Initialize() = 0;
	virtual void Invalidate() = 0;
	virtual void Record(const FrameInfo& frameInfo) = 0;
	virtual void Cleanup() = 0;
	virtual void Submit(uint32_t frameIndex) = 0;
};
