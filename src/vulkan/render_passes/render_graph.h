#pragma once
#include "core/uuid.h"
#include "render_pass.h"
#include "render_pass_resource.h"
#include <memory>
#include <unordered_map>
#include <limits>
#include <vector>
#include <iostream>
#include <algorithm>

class RenderGraph;

template <typename T>
class ResourceHandle
{
	friend class RenderGraph;

private:
	RenderGraph* m_Graph;
	size_t m_Id;

public:
	ResourceHandle() : m_Graph(nullptr), m_Id(std::numeric_limits<size_t>::max()) {}
	ResourceHandle(RenderGraph* graph, size_t id) : m_Graph(graph), m_Id(id) {}

	T* Get() const;

	size_t operator()() { return m_Id; }
	T* operator->() const { return Get(); }
	T& operator*() const { return *Get(); }

	explicit operator ResourceHandle<RenderPassResource>() const
	{
		static_assert(std::is_base_of<RenderPassResource, T>::value, "T must be derived from RenderPassResource");
		return {m_Graph, m_Id};
	}
};

class RenderGraph
{
public:
	using ResourceTable = std::unordered_map<std::string, std::vector<ResourceHandle<RenderPassResource>>>;
	struct PassResourceTables
	{
		ResourceTable ReadTable;
		ResourceTable WriteTable;
	};

	template <typename T, typename Factory, typename... Args>
	std::vector<ResourceHandle<T>> CreateResources(size_t count, const std::string& baseName, Factory&& factory, Args&&... args)
	{
		std::vector<ResourceHandle<T>> handles;
		handles.reserve(count);
		size_t startId = m_Resources.size();

		for (size_t i = 0; i < count; ++i)
		{
			auto resourceName = baseName + " " + std::to_string(i);
			auto resource = factory(resourceName, std::forward<Args>(args)...);
			m_Resources.push_back(std::move(resource));
			handles.push_back(ResourceHandle<T>(this, startId + i));
		}

		m_ResourceHandles[baseName] = static_cast<ResourceHandle<RenderPassResource>>(handles[0]);
		m_ResourceCounts[baseName] = count;
		return handles;
	}

	template <typename T, typename Factory, typename... Args>
	ResourceHandle<T> CreateResource(const std::string& baseName, Factory&& factory, Args&&... args)
	{
		auto handles = CreateResources<T>(1, baseName, factory, std::forward<Args>(args)...);
		return handles[0];
	}

	/// Gets the RenderGraphResource<T> using the handle ID.
	/// \tparam T
	/// \param id
	/// \return A pointer to the resource.
	template<typename T>
	T* Get(size_t id)
	{
		if (id >= m_Resources.size())
		{
			throw std::runtime_error("Invalid resource handle");
		}
		auto* resource = dynamic_cast<T*>(m_Resources[id].get());
		if (!resource)
		{
			throw std::bad_cast();
		}
		return resource;
	}

	template<typename T>
	T* Get(const std::string& baseName, uint32_t frameIndex = 0)
	{
		auto handleIt = m_ResourceHandles.find(baseName);
		auto countIt = m_ResourceCounts.find(baseName);
		if (handleIt == m_ResourceHandles.end() || countIt == m_ResourceCounts.end())
		{
			std::cout << "Resource not found: '" << baseName << "'\n";
			return nullptr;
		}

		size_t resourceIndex = handleIt->second() + (frameIndex % countIt->second);
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

	template<typename T>
	ResourceHandle<T> GetResourceHandle(const std::string& name)
	{
		auto it = m_ResourceHandles.find(name);
		if (it == m_ResourceHandles.end())
		{
			throw std::runtime_error("Resource not found: " + name);
		}
		return ResourceHandle<T>(this, it->second);
	}

	template<typename T>
	bool HasResource(const std::string& baseName, uint32_t* outCount = nullptr) const
	{
		auto handleIt = m_ResourceHandles.find(baseName);
		auto countIt = m_ResourceCounts.find(baseName);

		if (handleIt != m_ResourceHandles.end() && countIt != m_ResourceCounts.end())
		{
			if (outCount)
			{
				*outCount = static_cast<uint32_t>(countIt->second);
			}
			return true;
		}

		return false;
	}

	template<typename T, typename FreeFn, typename... Args>
	bool FreeResource(ResourceHandle<T> handle, FreeFn&& freeFn, Args&&... args)
	{
		size_t resourceIndex = handle.GetId();
		if (resourceIndex >= m_Resources.size())
		{
			std::cerr << "Invalid resource handle" << std::endl;
			return false;
		}

		// Attempt to cast the resource to the correct type
		auto resource = std::dynamic_pointer_cast<T>(m_Resources[resourceIndex]);
		if (!resource)
		{
			std::cerr << "Bad cast when trying to free resource" << std::endl;
			return false;
		}

		// Call the provided free function
		freeFn(resource->Get(), std::forward<Args>(args)...);

		// Remove the resource from the list and reset it
		m_Resources[resourceIndex].reset();

		// Erase the resource entry from the maps
		auto it = std::find_if(m_ResourceHandles.begin(), m_ResourceHandles.end(),
			[resourceIndex](const auto& pair) { return pair.second.GetId() == resourceIndex; });
		if (it != m_ResourceHandles.end())
		{
			std::string baseName = it->first;
			m_ResourceHandles.erase(it);
			m_ResourceCounts.erase(baseName);
		}

		// Adjust subsequent resource IDs in the handles map
		for (auto& pair : m_ResourceHandles)
		{
			if (pair.second() > resourceIndex)
			{
				pair.second = ResourceHandle<RenderPassResource>(this, pair.second() - 1);
			}
		}

		// Remove the entry from the m_Resources vector
		m_Resources.erase(m_Resources.begin() + resourceIndex);

		return true;
	}

	template <typename T, typename FreeFn, typename... Args>
	bool FreeResources(const std::string& baseName, uint32_t count, FreeFn&& freeFn, Args&&... args)
	{
		auto handleIt = m_ResourceHandles.find(baseName);
		auto countIt = m_ResourceCounts.find(baseName);

		if (handleIt == m_ResourceHandles.end() || countIt == m_ResourceCounts.end())
		{
			std::cerr << "Resource not found: " + baseName << "\n";
			return false;
		}

		size_t startId = handleIt->second();
		size_t endId = startId + count;

		for (size_t i = startId; i < endId; ++i)
		{
			if (i >= m_Resources.size())
			{
				std::cerr << "Invalid resource index at " << i << " when attempting to free " << baseName << "\n";
				return false;
			}

			auto resource = std::dynamic_pointer_cast<T>(m_Resources[i]);
			if (!resource)
			{
				std::cerr << "Bad cast when trying to free " << baseName << "\n";
				return false;
			}

			freeFn(resource->Get(), std::forward<Args>(args)...);
			m_Resources[i].reset();
		}

		// Remove the freed resources from our containers
		m_Resources.erase(m_Resources.begin() + startId, m_Resources.begin() + endId);
		m_ResourceHandles.erase(baseName);
		m_ResourceCounts.erase(baseName);

		// Update the IDs of the remaining resources
		for (auto& pair : m_ResourceHandles)
		{
			if (pair.second() > startId)
			{
				pair.second = ResourceHandle<RenderPassResource>(this, pair.second() - count);
			}
		}

		return true;
	}

	template <typename T, typename FreeFn, typename... Args>
	bool TryFreeResources(const std::string& baseName, FreeFn&& freeFn, Args&&... args)
	{
		uint32_t count = 0;
		bool has = HasResource<T>(baseName, &count);
		if(!has)
		{
			return false;
		}

		return FreeResources<T>(baseName, count, freeFn, std::forward<Args>(args)...);
	}

	ResourceTable ZeroInitializeResourceTable(const std::initializer_list<std::string>& declarations)
	{
		ResourceTable table;
		for(auto& declaration: declarations)
			table[declaration] = {};
		return table;
	}

	template <typename T>
	T* AddPass(const std::initializer_list<std::string>& readResources, const std::initializer_list<std::string>& writeResources)
	{
		// Create a unique_ptr to the derived pass type
		auto pass = std::make_unique<T>();
		uuid passUuid = pass->GetUuid();

		// Initialize the resource tables for the pass
		m_ResourceTable[passUuid] = {
			.ReadTable = ZeroInitializeResourceTable(readResources),
			.WriteTable = ZeroInitializeResourceTable(writeResources)
		};

		// Declare dependencies for the pass
		pass->DeclareDependencies(readResources, writeResources);

		// Store the raw pointer before moving the unique_ptr
		T* passPtr = pass.get();

		// Move the unique_ptr into the vector as a unique_ptr to RenderPass
		m_Passes[passUuid] = std::move(pass);

		// Return the raw pointer to the newly added pass
		return passPtr;
	}

	void ConstructDependencies(std::unordered_map<uuid, std::vector<uuid>>& outDependencies)
	{
		for (auto& [_, passA] : m_Passes)
		{
			for (auto& [_, passB] : m_Passes)
			{
				if(passA == passB) continue;
				bool passADependsOnB =
					std::any_of(passB->m_ReadResources.begin(), passB->m_ReadResources.end(),
					[&](const std::string& resource)
					{
						return find(passA->m_WriteResources.begin(), passA->m_WriteResources.end(), resource) != passA->m_WriteResources.end();
					});

				if(passADependsOnB)
				{
					outDependencies[passA->GetUuid()].push_back(passB->GetUuid());
				}
			}
		}
	}

	void OnSwapchainResize(uint32_t width, uint32_t height)
	{
		for (auto& [_, pass] : m_Passes)
		{
			pass->OnSwapchainResize(width, height, *this);
		}
	}

	void Initialize()
	{
		for (auto& [_, pass] : m_Passes)
		{
			pass->CreateResources(*this);
		}
	}

	void Execute(FrameInfo& frameInfo)
	{
		for (auto& [_, pass] : m_Passes)
		{
			pass->Record(frameInfo, *this);
			pass->Submit(frameInfo, *this);
		}
	}

private:
	std::vector<std::unique_ptr<RenderPassResource>> m_Resources;
	std::unordered_map<std::string, ResourceHandle<RenderPassResource>> m_ResourceHandles;
	std::unordered_map<std::string, size_t> m_ResourceCounts;

	std::unordered_map<uuid, std::unique_ptr<RenderPass>> m_Passes;

	std::unordered_map<uuid, PassResourceTables> m_ResourceTable;
};

template <typename T>
T* ResourceHandle<T>::Get() const
{
	if (!m_Graph)
	{
		throw std::runtime_error("Invalid resource handle");
	}
	return m_Graph->Get<T>(m_Id);
}