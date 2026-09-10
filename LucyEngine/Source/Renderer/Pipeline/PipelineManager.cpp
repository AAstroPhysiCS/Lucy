#include "lypch.h"
#include "PipelineManager.h"

#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "RayTracingPipeline.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	PipelineManager::PipelineManager(const Ref<RenderDevice>& device) 
		: m_RenderDevice(device) {
	}

	RenderDeviceResourceHandle PipelineManager::CreateGraphicsPipeline(const std::string& name, const Ref<Shader>& shader, const GraphicsPipelineCreateInfo& createInfo) {
		auto [tuple, success] = m_GraphicsPipelines.try_emplace(name, m_RenderDevice->CreateGraphicsPipeline(createInfo, shader));
		LUCY_ASSERT(success);
		return tuple->second;
	}

	RenderDeviceResourceHandle PipelineManager::CreateComputePipeline(const std::string& name, const Ref<Shader>& shader, const ComputePipelineCreateInfo& createInfo) {
		auto [tuple, success] = m_ComputePipelines.try_emplace(name, m_RenderDevice->CreateComputePipeline(createInfo, shader));
		LUCY_ASSERT(success);
		return tuple->second;
	}

	RenderDeviceResourceHandle PipelineManager::CreateRayTracingPipeline(const std::string& name, const RayTracingPipelineCreateInfo& createInfo) {
		auto [tuple, success] = m_RayTracingPipelines.try_emplace(name, m_RenderDevice->CreateRayTracingPipeline(createInfo));
		LUCY_ASSERT(success);
		return tuple->second;
	}

	std::unordered_map<std::string, GraphicsPipelineStatistics> PipelineManager::GetAllGraphicsPipelineStatistics() const {
		std::unordered_map<std::string, GraphicsPipelineStatistics> statistics;
		for (const auto& [name, handle] : m_GraphicsPipelines)
			statistics.try_emplace(name, m_RenderDevice->AccessResource<GraphicsPipeline>(handle)->GetStatistics());
		return statistics;
	}

	void PipelineManager::RTRecreateAllPipelinesDependentOnShader(const std::vector<Ref<Shader>>& shadersThatAreReloaded) {
		const auto RecreateAllPipelines = [&]<typename TPipeline>() {
			for (auto handle : (std::same_as<TPipeline, GraphicsPipeline>
				? m_GraphicsPipelines : std::same_as<TPipeline, ComputePipeline> ? m_ComputePipelines : m_RayTracingPipelines)
				| std::views::values) {
				const auto& pipeline = m_RenderDevice->AccessResource<TPipeline>(handle);
				for (const auto& shader : shadersThatAreReloaded) {
					if (pipeline->GetShader()->GetName() == shader->GetName()) {
						pipeline->RTRecreate(shader);
						break;
					}
				}
			}
		};

		RecreateAllPipelines.operator()<GraphicsPipeline>();
		RecreateAllPipelines.operator()<ComputePipeline>();
		RecreateAllPipelines.operator()<RayTracingPipeline>();
	}

	void PipelineManager::DestroyPipeline(const std::string& name) {
		if (m_GraphicsPipelines.contains(name)) {
			RenderDeviceResourceHandle handle = m_GraphicsPipelines.at(name);
			m_RenderDevice->RTDestroyResource(handle);
			m_GraphicsPipelines.erase(name);
			return;
		}
		if (m_ComputePipelines.contains(name)) {
			RenderDeviceResourceHandle handle = m_ComputePipelines.at(name);
			m_RenderDevice->RTDestroyResource(handle);
			m_ComputePipelines.erase(name);
			return;
		}
		LUCY_ASSERT(m_RayTracingPipelines.contains(name), "Destroying pipeline that does not exist in the cache!");
		RenderDeviceResourceHandle handle = m_RayTracingPipelines.at(name);
		m_RenderDevice->RTDestroyResource(handle);
		m_RayTracingPipelines.erase(name);
	}

	void PipelineManager::DestroyAll() {
		for (auto handle : m_GraphicsPipelines | std::views::values)
			Renderer::EnqueueResourceDestroy(handle);
		for (auto handle : m_ComputePipelines | std::views::values)
			Renderer::EnqueueResourceDestroy(handle);
		for (auto handle : m_RayTracingPipelines | std::views::values)
			Renderer::EnqueueResourceDestroy(handle);
		m_GraphicsPipelines.clear();
		m_ComputePipelines.clear();
		m_RayTracingPipelines.clear();
	}

	void PipelineManager::SaveToFileAsPSO() {
		//TODO:
	}

	void PipelineManager::ReadFromFileAsPSO() {
		//TODO:
	}
}