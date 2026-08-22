#pragma once

#include "Pipeline.h"

#include "Renderer/Shader/Shader.h"
#include "Renderer/Device/RenderDeviceHandles.h"

#include "PipelineConfigurations.h"

namespace Lucy {

	struct GraphicsPipelineCreateInfo {
		Topology Topology = Topology::TRIANGLES;
		Rasterization Rasterization;
		DepthConfiguration DepthConfiguration;
		BlendConfiguration BlendConfiguration;

		RenderDeviceResourceHandle RenderPassHandle;
	};

	class GraphicsPipelineStatistics {
	public:
		inline static constexpr const uint32_t PipelineStatSize = 8;

		GraphicsPipelineStatistics(std::vector<uint64_t>&& times = {});
		~GraphicsPipelineStatistics() = default;

		uint64_t GetInputAssemblyVertexCount() const { return m_Times[0]; }
		uint64_t GetInputAssemblyPrimitivesCount() const { return m_Times[1]; }
		uint64_t GetVertexShaderInvocations() const { return m_Times[2]; }
		uint64_t GetClippingStagePrimitivesProcessed() const { return m_Times[3]; }
		uint64_t GetClippingStagePrimitivesOutput() const { return m_Times[4]; }
		uint64_t GetFragmentShaderInvocations() const { return m_Times[5]; }
		uint64_t GetTesselationControlShaderPatches() const { return m_Times[6]; }
		uint64_t GetTesselationEvaluationShaderInvocations() const { return m_Times[7]; }

		constexpr bool IsEmpty() const { return m_Times.empty(); }
	private:
		std::vector<uint64_t> m_Times;
	};

	class GraphicsPipeline : public Pipeline {
	public:
		GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, Ref<Shader> shader);
		virtual ~GraphicsPipeline() = default;

		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
		GraphicsPipeline(GraphicsPipeline&&) = delete;
		GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

		inline Topology GetTopology() const { return m_CreateInfo.Topology; }
		inline Rasterization GetRasterization() const { return m_CreateInfo.Rasterization; }
		inline const GraphicsPipelineStatistics& GetStatistics() { return m_Statistics; }

		inline RenderDeviceResourceHandle GetRenderPassHandle() const { return m_CreateInfo.RenderPassHandle; }

		virtual void RTBind(void* commandBufferHandle) = 0;
		virtual void RTRecreate(Ref<Shader> newShader) = 0;
		void Unbind(GraphicsPipelineStatistics&& statistics);
	protected:
		GraphicsPipelineCreateInfo m_CreateInfo;
		GraphicsPipelineStatistics m_Statistics;
	};
}