#include "lypch.h"

#include "OpenGLPipeline.h"
#include "VulkanPipeline.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<Pipeline> Pipeline::Create(const PipelineCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLPipeline>(createInfo);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanPipeline>(createInfo);
				break;
			default:
				LUCY_CRITICAL("Other API's not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}

	Pipeline::Pipeline(const PipelineCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	PushConstant& Pipeline::GetPushConstants(const char* name) {
		for (PushConstant& pushConstant : m_PushConstants) {
			if (name == pushConstant.GetName()) {
				return pushConstant;
			}
		}
		LUCY_CRITICAL(fmt::format("Could not find a suitable Push Constant for the given name: {0}", name));
		LUCY_ASSERT(false);
	}

	uint32_t Pipeline::GetSizeFromType(ShaderDataSize size) {
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

	uint32_t Pipeline::CalculateStride(VertexShaderLayout vertexLayout) {
		uint32_t stride = 0;
		for (auto [name, size] : vertexLayout.ElementList) {
			stride += GetSizeFromType(size);
		}
		return stride;
	}

	void Pipeline::DestroyUniformBuffers() {
		for (const auto& uniformBuffer : m_UniformBuffers)
			uniformBuffer->DestroyHandle();
	}
}