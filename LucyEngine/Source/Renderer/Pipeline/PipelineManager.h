#pragma once

#include <unordered_map>

#include "Renderer/Device/RenderDevice.h"

namespace Lucy {

	class GraphicsPipelineStatistics;

	class PipelineManager final {
	public:
		PipelineManager(const Ref<RenderDevice>& device);
		~PipelineManager() = default;

		PipelineManager(const PipelineManager&) = delete;
		PipelineManager& operator=(const PipelineManager&) = delete;
		PipelineManager(PipelineManager&&) = delete;
		PipelineManager& operator=(PipelineManager&&) = delete;

		template <typename TPipeline>
		inline Ref<TPipeline> GetAs(const std::string& name) const {
			LUCY_PROFILE_NEW_EVENT("PipelineManager::GetAs");
			if (m_GraphicsPipelines.contains(name))
				return m_RenderDevice->AccessResource<TPipeline>(m_GraphicsPipelines.at(name));
			if (m_ComputePipelines.contains(name))
				return m_RenderDevice->AccessResource<TPipeline>(m_ComputePipelines.at(name));
			return m_RenderDevice->AccessResource<TPipeline>(m_RayTracingPipelines.at(name));
		}

		inline size_t GetGraphicsPipelineCount() const { return m_GraphicsPipelines.size(); }
		inline size_t GetComputePipelineCount() const { return m_ComputePipelines.size(); }
		inline size_t GetRayTracingPipelineCount() const { return m_RayTracingPipelines.size(); }
		inline size_t GetAllPipelineCount() const { return GetGraphicsPipelineCount() + GetComputePipelineCount() + GetRayTracingPipelineCount(); }

		std::unordered_map<std::string, GraphicsPipelineStatistics> GetAllGraphicsPipelineStatistics() const;

		void RTRecreateAllPipelinesDependentOnShader(const std::vector<Ref<Shader>>& shadersThatAreReloaded);

		void DestroyPipeline(const std::string& name);
		void DestroyAll();

		void SaveToFileAsPSO();
		void ReadFromFileAsPSO();
	private:
		RenderDeviceResourceHandle CreateGraphicsPipeline(const std::string& name, const Ref<Shader>& shader, const GraphicsPipelineCreateInfo& createInfo);
		RenderDeviceResourceHandle CreateComputePipeline(const std::string& name, const Ref<Shader>& shader, const ComputePipelineCreateInfo& createInfo);
		RenderDeviceResourceHandle CreateRayTracingPipeline(const std::string& name, const RayTracingPipelineCreateInfo& createInfo);

		std::unordered_map<std::string, RenderDeviceResourceHandle> m_GraphicsPipelines;
		std::unordered_map<std::string, RenderDeviceResourceHandle> m_ComputePipelines;
		std::unordered_map<std::string, RenderDeviceResourceHandle> m_RayTracingPipelines;

		Ref<RenderDevice> m_RenderDevice = nullptr;

		friend class Renderer; //for CreateGraphicsPipeline and CreateComputePipeline
	};
}

