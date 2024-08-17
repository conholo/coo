#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <utility>
#include <variant>

// Forward declarations
class VulkanTexture2D;
class VulkanBuffer;
class VulkanSampler;
class VulkanImageView;
class VulkanFramebuffer;
class VulkanRenderPass;
class VulkanGraphicsPipeline;

template <typename T>
class ResourceHandle
{
	friend class RenderGraph;

private:
	size_t m_Id;

public:
	explicit ResourceHandle(size_t id) : m_Id(id) {}

	ResourceHandle(const ResourceHandle&) = default;
	ResourceHandle& operator=(const ResourceHandle&) = default;

	ResourceHandle(ResourceHandle&&) noexcept = default;
	ResourceHandle& operator=(ResourceHandle&&) noexcept = default;

	size_t GetId() const { return m_Id; }
};

class RenderPassResource
{
public:
	enum class Type
	{
		Texture,
		Buffer,
		Sampler,
		ImageView,
		DescriptorSet,
		RenderPass,
		Framebuffer,
		GraphicsPipeline,
		CommandBuffer,
		Shader,
		MaterialLayout,
		Material
	};

	RenderPassResource(std::string name, Type type) : m_Name(std::move(name)), m_Type(type)
	{
	}
	virtual ~RenderPassResource() = default;

	const std::string& GetName() const
	{
		return m_Name;
	}

	Type GetType() const
	{
		return m_Type;
	}

private:
	std::string m_Name;
	Type m_Type;
};

class TextureResource : public RenderPassResource
{
public:
	TextureResource(const std::string& name, std::shared_ptr<VulkanTexture2D> texture)
		: RenderPassResource(name, Type::Texture), m_Texture(std::move(texture))
	{
	}
	TextureResource(TextureResource&& other) noexcept = default;
	TextureResource& operator=(TextureResource&& other) noexcept = default;

	std::shared_ptr<VulkanTexture2D> Get() const
	{
		return m_Texture;
	}

private:
	std::shared_ptr<VulkanTexture2D> m_Texture;
};

class ShaderResource : public RenderPassResource
{
public:
	ShaderResource(const std::string& name, std::shared_ptr<VulkanShader> shader)
		: RenderPassResource(name, Type::Shader), m_Shader(std::move(shader))
	{
	}
	ShaderResource(ShaderResource&& other) noexcept = default;
	ShaderResource& operator=(ShaderResource&& other) noexcept = default;

	std::shared_ptr<VulkanShader> Get() const
	{
		return m_Shader;
	}

private:
	std::shared_ptr<VulkanShader> m_Shader;
};

class MaterialLayoutResource : public RenderPassResource
{
public:
	MaterialLayoutResource(const std::string& name, std::shared_ptr<VulkanMaterialLayout> layout)
		: RenderPassResource(name, Type::MaterialLayout), m_Layout(std::move(layout))
	{
	}
	MaterialLayoutResource(MaterialLayoutResource&& other) noexcept = default;
	MaterialLayoutResource& operator=(MaterialLayoutResource&& other) noexcept = default;

	std::shared_ptr<VulkanMaterialLayout> Get() const
	{
		return m_Layout;
	}

private:
	std::shared_ptr<VulkanMaterialLayout> m_Layout;
};

class MaterialResource : public RenderPassResource
{
public:
	MaterialResource(const std::string& name, std::shared_ptr<VulkanMaterial> material)
		: RenderPassResource(name, Type::Material), m_Material(std::move(material))
	{
	}
	MaterialResource(MaterialResource&& other) noexcept = default;
	MaterialResource& operator=(MaterialResource&& other) noexcept = default;

	std::shared_ptr<VulkanMaterial> Get() const
	{
		return m_Material;
	}

private:
	std::shared_ptr<VulkanMaterial> m_Material;
};

class SemaphoreResource : public RenderPassResource
{
public:
	SemaphoreResource(const std::string& name, VkSemaphore semaphore)
		: RenderPassResource(name, Type::Texture), m_Semaphore(semaphore)
	{
	}

	VkSemaphore Get() const
	{
		return m_Semaphore;
	}

private:
	VkSemaphore m_Semaphore;
};

class CommandBufferResource : public RenderPassResource
{
public:
	CommandBufferResource(const std::string& name, std::shared_ptr<VulkanCommandBuffer> cmdBuffer)
		: RenderPassResource(name, Type::Texture), m_CommandBuffer(std::move(cmdBuffer))
	{
	}

	std::shared_ptr<VulkanCommandBuffer> Get() const
	{
		return m_CommandBuffer;
	}

private:
	std::shared_ptr<VulkanCommandBuffer> m_CommandBuffer;
};

class BufferResource : public RenderPassResource
{
public:
	BufferResource(const std::string& name, std::shared_ptr<VulkanBuffer> buffer)
		: RenderPassResource(name, Type::Buffer), m_Buffer(std::move(buffer))
	{
	}

	std::shared_ptr<VulkanBuffer> Get() const
	{
		return m_Buffer;
	}

private:
	std::shared_ptr<VulkanBuffer> m_Buffer;
};

class SamplerResource : public RenderPassResource
{
public:
	SamplerResource(const std::string& name, std::shared_ptr<VulkanSampler> sampler)
		: RenderPassResource(name, Type::Sampler), m_Sampler(std::move(sampler))
	{
	}

	std::shared_ptr<VulkanSampler> Get() const
	{
		return m_Sampler;
	}

private:
	std::shared_ptr<VulkanSampler> m_Sampler;
};

class ImageViewResource : public RenderPassResource
{
public:
	ImageViewResource(const std::string& name, std::shared_ptr<VulkanImageView> imageView)
		: RenderPassResource(name, Type::ImageView), m_ImageView(std::move(imageView))
	{
	}

	std::shared_ptr<VulkanImageView> Get() const
	{
		return m_ImageView;
	}

private:
	std::shared_ptr<VulkanImageView> m_ImageView;
};

class DescriptorSetResource : public RenderPassResource
{
public:
	DescriptorSetResource(const std::string& name, VkDescriptorSet descriptorSet)
		: RenderPassResource(name, Type::DescriptorSet), m_DescriptorSet(descriptorSet)
	{
	}

	VkDescriptorSet GetDescriptorSet() const
	{
		return m_DescriptorSet;
	}

private:
	VkDescriptorSet m_DescriptorSet;
};

class RenderPassObjectResource : public RenderPassResource
{
public:
	RenderPassObjectResource(const std::string& name, std::shared_ptr<VulkanRenderPass> renderPass)
		: RenderPassResource(name, Type::RenderPass), m_RenderPass(std::move(renderPass))
	{
	}

	std::shared_ptr<VulkanRenderPass> Get() const
	{
		return m_RenderPass;
	}

private:
	std::shared_ptr<VulkanRenderPass> m_RenderPass;
};

class GraphicsPipelineObjectResource : public RenderPassResource
{
public:
	GraphicsPipelineObjectResource(const std::string& name, std::shared_ptr<VulkanGraphicsPipeline> pipeline)
		: RenderPassResource(name, Type::GraphicsPipeline), m_Pipeline(std::move(pipeline))
	{
	}

	std::shared_ptr<VulkanGraphicsPipeline> Get() const
	{
		return m_Pipeline;
	}

private:
	std::shared_ptr<VulkanGraphicsPipeline> m_Pipeline;
};

class FramebufferResource : public RenderPassResource
{
public:
	FramebufferResource(const std::string& name, std::shared_ptr<VulkanFramebuffer> framebuffer)
		: RenderPassResource(name, Type::Framebuffer), m_Framebuffer(std::move(framebuffer))
	{
	}

	std::shared_ptr<VulkanFramebuffer> Get() const
	{
		return m_Framebuffer;
	}

private:
	std::shared_ptr<VulkanFramebuffer> m_Framebuffer;
};