#include "lypch.h"
#include "GraphicsPipeline.h"

namespace Lucy {

	GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, Ref<Shader> shader)
		: Pipeline("GraphicsPipeline", shader), m_CreateInfo(createInfo) {
	}

	void GraphicsPipeline::Unbind(GraphicsPipelineStatistics&& statistics) {
		m_Statistics = std::move(statistics);
	}

	GraphicsPipelineStatistics::GraphicsPipelineStatistics(std::vector<uint64_t>&& times) 
		: m_Times(std::move(times)) {
	}
}