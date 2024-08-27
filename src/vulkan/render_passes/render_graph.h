#pragma once
#include "core/uuid.h"
#include "render_pass.h"
#include "render_pass_resource.h"
#include <memory>
#include <unordered_map>
#include <limits>
#include <vector>
#include <queue>
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
			auto resource = factory(i, resourceName, std::forward<Args>(args)...);
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
	ResourceHandle<T> GetGlobalResourceHandle(const std::string& name, uint32_t frameIndex = 0)
	{
		auto it = m_ResourceHandles.find(name);
		if (it == m_ResourceHandles.end())
		{
			throw std::runtime_error("Resource not found: " + name);
		}

		// Determine the correct resource index based on the frame index and buffering
		size_t baseIndex = it->second();
		auto countIt = m_ResourceCounts.find(name);
		if (countIt == m_ResourceCounts.end())
		{
			throw std::runtime_error("Resource count not found for: " + name);
		}

		// Compute the resource handle based on the frame index and the number of resources
		size_t resourceIndex = baseIndex + (frameIndex % countIt->second);

		return ResourceHandle<T>(this, resourceIndex);
	}

	template<typename T>
	ResourceHandle<T> GetResourceHandle(const std::string& name, const uuid& passUuid, uint32_t frameIndex = 0)
	{
		auto it = m_ResourceHandles.find(name);
		if (it == m_ResourceHandles.end())
		{
			throw std::runtime_error("Resource not found: " + name);
		}

		// Check if the requesting pass has this resource declared as a read dependency
		const auto& passResources = m_ResourceTable[passUuid];
		if (passResources.ReadTable.find(name) == passResources.ReadTable.end())
		{
			throw std::runtime_error("Pass does not have read access to the requested resource: " + name);
		}

		// Determine the correct resource index based on the frame index and buffering
		size_t baseIndex = it->second();
		auto countIt = m_ResourceCounts.find(name);
		if (countIt == m_ResourceCounts.end())
		{
			throw std::runtime_error("Resource count not found for: " + name);
		}

		size_t resourceIndex = baseIndex + (frameIndex % countIt->second);
		return ResourceHandle<T>(this, resourceIndex);
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
		std::unordered_map<uuid, std::vector<uuid>> dependencies;
		ConstructDependencies(dependencies);

		std::vector<RenderPass*> sortedPasses;
		std::unordered_map<uuid, int> inDegree;

		for(auto& dep: dependencies)
		{
			for(uuid depPassUuid : dep.second)
			{
				RenderPass* pass = m_Passes[depPassUuid].get();
				inDegree[pass->GetUuid()]++;
			}
		}

		std::queue<RenderPass*> readyQueue;
		for(auto& [_, pass] : m_Passes)
		{
			if(inDegree[pass->GetUuid()] == 0)
			{
				readyQueue.push(pass.get());
			}
		}

		while(!readyQueue.empty())
		{
			RenderPass* pass = readyQueue.front();
			readyQueue.pop();
			sortedPasses.push_back(pass);

			for(auto& dependentUuid: dependencies[pass->GetUuid()])
			{
				if(--inDegree[dependentUuid] == 0)
				{
					readyQueue.push(m_Passes[dependentUuid].get());
				}
			}
		}

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