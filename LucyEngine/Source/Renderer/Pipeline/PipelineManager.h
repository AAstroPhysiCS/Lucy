#pragma once

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

	class PipelineManager final {
	public:
		PipelineManager(const Ref<RenderDevice>& device);
		~PipelineManager() = default;

		template <typename TPipeline>
		inline Ref<TPipeline> GetAs(const std::string& name) {
			if (m_GraphicsPipelines.contains(name))
				return m_RenderDevice->AccessResource<TPipeline>(m_GraphicsPipelines.at(name));
			return m_RenderDevice->AccessResource<TPipeline>(m_ComputePipelines.at(name));
		}

		inline size_t GetGraphicsPipelineCount() const { return m_GraphicsPipelines.size(); }
		inline size_t GetComputePipelineCount() const { return m_ComputePipelines.size(); }
		inline size_t GetAllPipelineCount() const { return GetGraphicsPipelineCount() + GetComputePipelineCount(); }

		void RTRecreateAllPipelinesDependentOnShader(const Ref<Shader>& shader);

		void DestroyPipeline(const std::string& name);
		void RTDestroyAll();

		void SaveToFileAsPSO();
		void ReadFromFileAsPSO();
	private:
		RenderResourceHandle CreateGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo);
		RenderResourceHandle CreateComputePipeline(const std::string& name, const ComputePipelineCreateInfo& createInfo);

		std::unordered_map<std::string, RenderResourceHandle> m_GraphicsPipelines;
		std::unordered_map<std::string, RenderResourceHandle> m_ComputePipelines;

		Ref<RenderDevice> m_RenderDevice = nullptr;

		friend class Renderer; //for CreateGraphicsPipeline and CreateComputePipeline
	};
}

