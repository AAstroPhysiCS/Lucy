#include "lypch.h"
#include "PipelineManager.h"

#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

namespace Lucy {

	PipelineManager::PipelineManager(const Ref<RenderDevice>& device) 
		: m_RenderDevice(device) {
	}

	RenderResourceHandle PipelineManager::CreateGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo) {
		auto [tuple, success] = m_GraphicsPipelines.try_emplace(name, m_RenderDevice->CreateGraphicsPipeline(createInfo));
		LUCY_ASSERT(success);
		return tuple->second;
	}

	RenderResourceHandle PipelineManager::CreateComputePipeline(const std::string& name, const ComputePipelineCreateInfo& createInfo) {
		auto [tuple, success] = m_ComputePipelines.try_emplace(name, m_RenderDevice->CreateComputePipeline(createInfo));
		LUCY_ASSERT(success);
		return tuple->second;
	}

	void PipelineManager::RTRecreateAllPipelinesDependentOnShader(const Ref<Shader>& shader) {
		const auto RecreateAllPipelines = [&]<typename TPipeline>() {
			for (auto handle : (std::same_as<TPipeline, GraphicsPipeline>
				? m_GraphicsPipelines : m_ComputePipelines)
				| std::views::values) {
				const auto& pipeline = m_RenderDevice->AccessResource<TPipeline>(handle);
				if (pipeline->GetShader() == shader)
					pipeline->RTRecreate();
			}
		};

		RecreateAllPipelines.operator()<GraphicsPipeline>();
		RecreateAllPipelines.operator()<ComputePipeline>();
	}

	void PipelineManager::DestroyPipeline(const std::string& name) {
		if (m_GraphicsPipelines.contains(name)) {
			RenderResourceHandle handle = m_GraphicsPipelines.at(name);
			m_RenderDevice->RTDestroyResource(handle);
			m_GraphicsPipelines.erase(name);
			return;
		}
		LUCY_ASSERT(m_ComputePipelines.contains(name), "Destroying pipeline that does not exist in the cache!");
		RenderResourceHandle handle = m_ComputePipelines.at(name);
		m_RenderDevice->RTDestroyResource(handle);
		m_ComputePipelines.erase(name);
	}

	void PipelineManager::RTDestroyAll() {
		for (auto handle : m_GraphicsPipelines | std::views::values)
			m_RenderDevice->RTDestroyResource(handle);
		for (auto handle : m_ComputePipelines | std::views::values)
			m_RenderDevice->RTDestroyResource(handle);
		m_GraphicsPipelines.clear();
		m_ComputePipelines.clear();
	}

	void PipelineManager::SaveToFileAsPSO() {
		//TODO:
	}

	void PipelineManager::ReadFromFileAsPSO() {
		//TODO:
	}
}