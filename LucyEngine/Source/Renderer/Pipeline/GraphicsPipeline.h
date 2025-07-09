#pragma once

#include "Renderer/Device/RenderResource.h"
#include "Renderer/Shader/Shader.h"

#include "PipelineConfigurations.h"

namespace Lucy {

	struct GraphicsPipelineCreateInfo {
		Topology Topology = Topology::TRIANGLES;
		Rasterization Rasterization;
		DepthConfiguration DepthConfiguration;
		BlendConfiguration BlendConfiguration;
		VertexShaderLayout VertexShaderLayout;

		RenderResourceHandle RenderPassHandle;
		Ref<Shader> Shader;
	};

	class GraphicsPipelineStatistics {
	public:
		inline static constexpr const uint32_t PipelineStatSize = 8;

		GraphicsPipelineStatistics(std::vector<uint64_t>&& times = {});
		~GraphicsPipelineStatistics() = default;

		uint64_t GetInputAssemblyVertexCount() const { return GetStatisticsOrZero(0); }
		uint64_t GetInputAssemblyPrimitivesCount() const { return GetStatisticsOrZero(1); }
		uint64_t GetVertexShaderInvocations() const { return GetStatisticsOrZero(2); }
		uint64_t GetClippingStagePrimitivesProcessed() const { return GetStatisticsOrZero(3); }
		uint64_t GetClippingStagePrimitivesOutput() const { return GetStatisticsOrZero(4); }
		uint64_t GetFragmentShaderInvocations() const { return GetStatisticsOrZero(5); }
		uint64_t GetTesselationControlShaderPatches() const { return GetStatisticsOrZero(6); }
		uint64_t GetTesselationEvaluationShaderInvocations() const { return GetStatisticsOrZero(7); }
	private:
		std::vector<uint64_t> m_Times;

		uint64_t GetStatisticsOrZero(size_t index) const {
			return (index < m_Times.size()) ? m_Times[index] : 0;
		}
	};


	class GraphicsPipeline : public RenderResource {
	public:
		GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
		virtual ~GraphicsPipeline() = default;

		inline const Ref<Shader>& GetShader() const { return m_CreateInfo.Shader; }

		inline Topology GetTopology() const { return m_CreateInfo.Topology; }
		inline Rasterization GetRasterization() const { return m_CreateInfo.Rasterization; }
		inline const GraphicsPipelineStatistics& GetStatistics() { return m_Statistics; }

		inline RenderResourceHandle GetRenderPassHandle() const { return m_CreateInfo.RenderPassHandle; }

		virtual void RTBind(void* commandBufferHandle) = 0;
		virtual void RTRecreate() = 0;
		void Unbind(GraphicsPipelineStatistics&& statistics);
	protected:
		static uint32_t CalculateStride(const VertexShaderLayout& vertexLayout);

		GraphicsPipelineCreateInfo m_CreateInfo;
		GraphicsPipelineStatistics m_Statistics;
	};
}