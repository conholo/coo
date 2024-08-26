#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_texture.h"

class TextureResource : public RenderPassResource
{
public:
	explicit TextureResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	TextureResource(const std::string& name, VulkanTexture2D* texture2D)
		: RenderPassResource(name), m_Texture(texture2D)
	{
	}

	~TextureResource()
	{
		delete m_Texture;
	}

	VulkanTexture2D& Get() const
	{
		return *m_Texture;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Texture;
		m_Texture = new VulkanTexture2D(std::forward<Args>(args)...);
	}

private:
	VulkanTexture2D* m_Texture = nullptr;
};
