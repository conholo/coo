#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_image.h"

class ImageResource : public RenderPassResource
{
public:
	explicit ImageResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	ImageResource(const std::string& name, VulkanImage2D* image)
		: RenderPassResource(name), m_Image2D(image)
	{
	}

	~ImageResource()
	{
		delete m_Image2D;
	}

	VulkanImage2D& Get() const
	{
		return *m_Image2D;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Image2D;
		m_Image2D = new VulkanImage2D(std::forward<Args>(args)...);
	}

private:
	VulkanImage2D* m_Image2D = nullptr;
};
