#include "lypch.h"
#include "Pipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Shader/ShaderReflect.h"

namespace Lucy {
	
	Pipeline::Pipeline(std::string_view name, Ref<Shader> shader)
		: RenderDeviceResource(name), m_Shader(shader) {
	}

	PipelineConstant& Pipeline::GetPipelineConstants(const std::string& name) {
		LUCY_PROFILE_NEW_EVENT("Pipeline::GetPipelineConstants");
		auto it = std::ranges::find_if(m_PushConstants, [&name](const PipelineConstant& pushConstant) {
			return pushConstant.GetName() == name;
		});
		return *it;
		LUCY_ASSERT(false, "Could not find a suitable Push Constant for the given name: {0}", name);
	}

	void Pipeline::RTDestroyResource(RenderDevice* device) {
		m_PushConstants.clear();
	}

	void Pipeline::AddPushConstant(const ShaderVariable& pc) {
		auto it = std::ranges::find_if(m_PushConstants, [&pc](const PipelineConstant& pushConstant) {
			return pushConstant.GetName() == pc.Name;
		});

		if (it == m_PushConstants.end()) {
			m_PushConstants.emplace_back(pc.Name, pc.BufferSize, 0, pc.StageFlag);
			return;
		}

		VkPushConstantRange range = it->GetHandle();
		
		uint32_t size = std::max(range.size, pc.BufferSize);
		VkShaderStageFlags stageFlags = range.stageFlags | pc.StageFlag;

		*it = PipelineConstant(pc.Name, size, range.offset, stageFlags);
	}
}
