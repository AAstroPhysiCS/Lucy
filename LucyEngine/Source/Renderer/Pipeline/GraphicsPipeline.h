#pragma once

#include "Renderer/Device/RenderResource.h"
#include "Renderer/Shader/Shader.h"

#include "PipelineConfigurations.h"

namespace Lucy {

	struct GraphicsPipelineCreateInfo {
		Topology Topology = Topology::TRIANGLES;
		Rasterization Rasterization;
		DepthConfiguration DepthConfiguration;
		VertexShaderLayout VertexShaderLayout;

		RenderResourceHandle RenderPassHandle;
		Ref<Shader> Shader;
	};

	using GraphicsPipelineStatistics = std::vector<uint64_t>;

	class GraphicsPipeline : public RenderResource {
	public:
		GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
		virtual ~GraphicsPipeline() = default;

		inline const Ref<Shader>& GetShader() const { return m_CreateInfo.Shader; }

		inline Topology GetTopology() const { return m_CreateInfo.Topology; }
		inline Rasterization GetRasterization() const { return m_CreateInfo.Rasterization; }

		inline RenderResourceHandle GetRenderPassHandle() const { return m_CreateInfo.RenderPassHandle; }

		virtual void RTBind(void* commandBufferHandle) = 0;
		virtual void RTRecreate() = 0;
		void Unbind(const GraphicsPipelineStatistics& statistics);
	protected:
		static uint32_t CalculateStride(const VertexShaderLayout& vertexLayout);

		GraphicsPipelineCreateInfo m_CreateInfo;
		GraphicsPipelineStatistics m_Statistics;
	};
}