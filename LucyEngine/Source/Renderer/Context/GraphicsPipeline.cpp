#include "lypch.h"
#include "VulkanGraphicsPipeline.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanGraphicsPipeline>(createInfo);
				break;
			default:
				LUCY_CRITICAL("Other API's not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}

	GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	VulkanPushConstant& GraphicsPipeline::GetPushConstants(const char* name) {
		for (VulkanPushConstant& pushConstant : m_PushConstants) {
			if (name == pushConstant.GetName()) {
				return pushConstant;
			}
		}
		LUCY_CRITICAL(fmt::format("Could not find a suitable Push Constant for the given name: {0}", name));
		LUCY_ASSERT(false);
	}

	uint32_t GraphicsPipeline::GetSizeFromType(ShaderDataSize size) {
		switch (size) {
			case ShaderDataSize::Int1:
			case ShaderDataSize::Float1: return 1; break;
			case ShaderDataSize::Int2:
			case ShaderDataSize::Float2: return 2; break;
			case ShaderDataSize::Int3:
			case ShaderDataSize::Float3: return 3; break;
			case ShaderDataSize::Int4:
			case ShaderDataSize::Float4: return 4; break;
			case ShaderDataSize::Mat4:	 return 4 * 4; break;
		}
		return 0;
	}

	uint32_t GraphicsPipeline::CalculateStride(VertexShaderLayout vertexLayout) {
		uint32_t stride = 0;
		for (const auto& [name, size] : vertexLayout) {
			stride += GetSizeFromType(size);
		}
		return stride;
	}
}