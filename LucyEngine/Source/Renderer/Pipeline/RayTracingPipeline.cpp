#include "lypch.h"
#include "RayTracingPipeline.h"

namespace Lucy {
	
	RayTracingPipeline::RayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) 
		: Pipeline("RayTracingPipeline", createInfo.RayGenShader), m_CreateInfo(createInfo) {
	}

	AccelerationStructure::AccelerationStructure(const BLAccelerationStructureCreateInfo& createInfo, AccelerationStructureType type) 
		: RenderDeviceResource("BottomLevelAccelerationStructure"), m_BLCreateInfo(createInfo), m_Type(type) {
	}

	AccelerationStructure::AccelerationStructure(const TLAccelerationStructureCreateInfo& createInfo, AccelerationStructureType type)
		: RenderDeviceResource("TopLevelAccelerationStructure"), m_TLCreateInfo(createInfo), m_Type(type) {
	}
}
