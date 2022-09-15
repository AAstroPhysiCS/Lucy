#include "lypch.h"
#include "ComputePipeline.h"

#include "VulkanComputePipeline.h"
#include "Renderer/Renderer.h"

namespace Lucy {
	
	Ref<ComputePipeline> ComputePipeline::Create(const Ref<ComputeShader>& computeShader) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanComputePipeline>(computeShader);
				break;
		}
		return nullptr;
	}

	ComputePipeline::ComputePipeline(const Ref<ComputeShader>& computeShader)
		: m_ComputeShader(computeShader) {
	}

	VulkanPushConstant& ComputePipeline::GetPushConstants(const char* name) {
		for (VulkanPushConstant& pushConstant : m_PushConstants) {
			if (name == pushConstant.GetName()) {
				return pushConstant;
			}
		}
		LUCY_CRITICAL(fmt::format("Could not find a suitable Push Constant for the given name: {0}", name));
		LUCY_ASSERT(false);
	}
}