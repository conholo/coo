#pragma once

#include "render_pass_resource.h"

#include <vector>

class RenderGraph;

class RenderPass
{
public:
	virtual void CreateResources(RenderGraph& graph) = 0;
	virtual void Record(const FrameInfo& frameInfo, RenderGraph& graph) = 0;
	virtual void Submit(const FrameInfo& frameInfo, RenderGraph& graph) = 0;
private:
	friend class RenderGraph;
};