#include "lypch.h"
#include "ComputePipeline.h"

namespace Lucy {
	
	ComputePipeline::ComputePipeline(const ComputePipelineCreateInfo& createInfo)
		: ContextPipeline(ContextPipelineType::Compute), m_CreateInfo(createInfo) {
	}
}