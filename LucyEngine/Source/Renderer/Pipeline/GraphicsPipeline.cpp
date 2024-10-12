#include "lypch.h"
#include "GraphicsPipeline.h"

namespace Lucy {

	GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
		: RenderResource("GraphicsPipeline"), m_CreateInfo(createInfo) {
	}

	void GraphicsPipeline::Unbind(const GraphicsPipelineStatistics& statistics) {
		m_Statistics = statistics;
	}

	uint32_t GraphicsPipeline::CalculateStride(const VertexShaderLayout& vertexLayout) {
		uint32_t stride = 0;
		for (const auto& [name, location, type, size] : vertexLayout)
			stride += size;
		return stride;
	}
}