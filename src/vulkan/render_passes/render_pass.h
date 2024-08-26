#pragma once

#include "render_pass_resource.h"

struct FrameInfo;
class RenderGraph;

class RenderPass
{
public:
	virtual ~RenderPass() = default;
	virtual void DeclareDependencies(const std::initializer_list<std::string>& readResources, const std::initializer_list<std::string>& writeResources) = 0;
	virtual void CreateResources(RenderGraph& graph) = 0;
	virtual void Record(const FrameInfo& frameInfo, RenderGraph& graph) = 0;
	virtual void Submit(const FrameInfo& frameInfo, RenderGraph& graph) = 0;
	virtual void OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph) = 0;

protected:
	std::vector<std::string> m_ReadResources;
	std::vector<std::string> m_WriteResources;

private:
	friend class RenderGraph;
};

