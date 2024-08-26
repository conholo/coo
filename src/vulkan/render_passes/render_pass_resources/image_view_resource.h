#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_image.h"

class ImageViewResource : public RenderPassResource
{
public:
	explicit ImageViewResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	ImageViewResource(const std::string& name, VulkanImageView* imageView)
		: RenderPassResource(name), m_ImageView(imageView)
	{
	}

	~ImageViewResource()
	{
		delete m_ImageView;
	}

	VulkanImageView& Get() const
	{
		return *m_ImageView;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_ImageView;
		m_ImageView = new VulkanImageView(std::forward<Args>(args)...);
	}

private:
	VulkanImageView* m_ImageView = nullptr;
};
