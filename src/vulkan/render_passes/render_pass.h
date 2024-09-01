#pragma once

#include "render_pass_resource.h"

#include <core/uuid.h>

struct FrameInfo;
class RenderGraph;

class RenderPass
{
public:
	virtual ~RenderPass() = default;

	virtual void DeclareDependencies(const std::initializer_list<std::string>& readResources, const std::initializer_list<std::string>& writeResources)
	{
		m_ReadResources = {readResources};
		m_WriteResources = {writeResources};
	}

	virtual void CreateResources(RenderGraph& graph) = 0;
	virtual void Record(const FrameInfo& frameInfo, RenderGraph& graph) = 0;
	virtual void Submit(const FrameInfo& frameInfo, RenderGraph& graph) = 0;
	virtual void OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph) = 0;
	uuid GetUuid() { return m_uuid; }

protected:
	std::vector<std::string> m_ReadResources;
	std::vector<std::string> m_WriteResources;
	uuid m_uuid;

private:
	friend class RenderGraph;
};

