#include "lypch.h"
#include "Pipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Shader/ShaderReflect.h"

namespace Lucy {
	
	Pipeline::Pipeline(std::string_view name, Ref<Shader> shader)
		: RenderDeviceResource(name), m_Shader(shader) {
	}

	PipelineConstant& Pipeline::GetPipelineConstants(const std::string& name) {
		for (PipelineConstant& pushConstant : m_PushConstants) {
			if (name == pushConstant.GetName()) {
				return pushConstant;
			}
		}
		LUCY_ASSERT(false, "Could not find a suitable Push Constant for the given name: {0}", name);
	}

	void Pipeline::RTDestroyResource(RenderDevice* device) {
		m_PushConstants.clear();
	}

	void Pipeline::AddPushConstant(const ShaderVariable& pc) {
		m_PushConstants.emplace_back(pc.Name, pc.BufferSize, 0, pc.StageFlag);
	}
}
