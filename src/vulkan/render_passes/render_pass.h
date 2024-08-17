#pragma once

#include "render_pass_resource.h"

#include <vector>

class RenderGraph;

class RenderPass
{
public:
	virtual void CreateResources(RenderGraph& graph) = 0;
	virtual void Resize(RenderGraph& graph, uint32_t width, uint32_t height) = 0;
	virtual void RecordCommandBuffer(RenderGraph& graph, VkCommandBuffer cmd, uint32_t frameIndex) = 0;
	virtual void Cleanup(RenderGraph& graph) = 0;
	virtual void SetDependencies(RenderGraph& graph) = 0;

private:
	friend class RenderGraph;
};