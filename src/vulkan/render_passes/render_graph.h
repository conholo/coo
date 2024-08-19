#pragma once

#include "render_pass.h"
#include "render_pass_resource.h"

#include <memory>
#include <unordered_map>
#include <vector>

class RenderGraph
{
public:
	template <typename T, typename Factory, typename... Args>
	std::vector<ResourceHandle<T>> CreateResources(size_t count, const std::string& baseName, Factory&& factory, Args&&... args)
	{
		// Ensure T is RenderPassResource
		static_assert(std::is_base_of<RenderPassResource, T>::value, "T must be derived from RenderPassResource");

		std::vector<ResourceHandle<T>> handles;
		handles.reserve(count);
		size_t startId = m_Resources.size();

		for (size_t i = 0; i < count; ++i)
		{
			auto resource = factory(i, baseName, std::forward<Args>(args)...);
			m_Resources.push_back(std::move(resource));
			handles.push_back(ResourceHandle<T>(startId + i));
		}

		// Avoid the need for a default constructor of ResourceHandle by using emplace - construct in place
		m_ResourceHandles.emplace(baseName, ResourceHandle<RenderPassResource>(startId));
		m_ResourceCounts[baseName] = count;
		return handles;
	}

	template <typename T, typename Factory, typename... Args>
	ResourceHandle<T> CreateResource(const std::string& baseName, Factory&& factory, Args&&... args)
	{
		// Ensure T is derived from RenderPassResource
		static_assert(std::is_base_of<RenderPassResource, T>::value, "T must be derived from RenderPassResource");

		size_t startId = m_Resources.size();  // Get the current size to use as the ID
		auto resource = factory(baseName, std::forward<Args>(args)...);
		m_Resources.push_back(std::move(resource));  // Store the created resource

		// Emplace the resource handle in the map, using T for correct type handling
		m_ResourceHandles.emplace(baseName, ResourceHandle<RenderPassResource>(startId));
		m_ResourceCounts[baseName] = 1;  // Set the resource count for the baseName

		return ResourceHandle<T>(startId);  // Return the handle to the created resource
	}

	template<typename T>
	T* GetResource(ResourceHandle<T> handle)
	{
		size_t resourceIndex = handle.GetId();
		if (resourceIndex >= m_Resources.size())
		{
			throw std::out_of_range("Invalid resource handle");
		}

		auto* resource = dynamic_cast<T*>(m_Resources[resourceIndex].get());
		if (!resource)
		{
			throw std::bad_cast();
		}
		return resource;
	}

	template<typename T>
	T* GetResource(const std::string& baseName, uint32_t frameIndex = 0)
	{
		auto handleIt = m_ResourceHandles.find(baseName);
		auto countIt = m_ResourceCounts.find(baseName);
		if (handleIt == m_ResourceHandles.end() || countIt == m_ResourceCounts.end())
		{
			throw std::runtime_error("Resource not found: " + baseName);
		}

		size_t resourceIndex = handleIt->second.GetId() + (frameIndex % countIt->second);
		if (resourceIndex >= m_Resources.size())
		{
			throw std::out_of_range("Invalid resource index");
		}

		auto* resource = dynamic_cast<T*>(m_Resources[resourceIndex].get());
		if (!resource)
		{
			throw std::bad_cast();
		}
		return resource;
	}

	template <typename T>
	T* AddPass(std::function<std::unique_ptr<T>()> factory)
	{
		auto pass = factory();
		T* ptr = pass.get();
		m_Passes.push_back(std::move(pass));
		return ptr;
	}

	template <typename T>
	T* AddPass()
	{
		auto pass = std::make_unique<T>();
		T* ptr = pass.get();
		m_Passes.push_back(std::move(pass));
		return ptr;
	}

	template <typename T, typename... Args>
	T* AddPass(Args&&... args)
	{
		return AddPass<T>([&]() { return std::make_unique<T>(std::forward<Args>(args)...); });
	}
	
	void Initialize()
	{
		for (auto& pass : m_Passes)
		{
			pass->CreateResources(*this);
		}
	}

	void Execute(FrameInfo& frameInfo)
	{
		for (auto& pass : m_Passes)
		{
			pass->Record(frameInfo, *this);
			pass->Submit(frameInfo, *this);
		}
	}

private:
	std::vector<std::shared_ptr<RenderPassResource>> m_Resources;
	std::unordered_map<std::string, ResourceHandle<RenderPassResource>> m_ResourceHandles;
	std::unordered_map<std::string, size_t> m_ResourceCounts;
	std::vector<std::unique_ptr<RenderPass>> m_Passes;
};