#include "lypch.h"
#include "OpenGLPipeline.h"
#include "VulkanPipeline.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	
	RefLucy<Pipeline> Pipeline::Create(const PipelineSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLPipeline>(specs);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanPipeline>(specs);
				break;
			default:
				LUCY_CRITICAL("Other API's not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}

	Pipeline::Pipeline(const PipelineSpecification& specs)
		: m_Specs(specs) {
	}

	void Pipeline::Begin(const RefLucy<Pipeline>& pipeline) {
		pipeline->BeginVirtual();
		s_ActivePipeline = pipeline.get();
	}

	void Pipeline::End(const RefLucy<Pipeline>& pipeline) {
		pipeline->EndVirtual();
		s_ActivePipeline = nullptr;
	}

	void Pipeline::DestroyUniformBuffers() {
		for (const auto& uniformBuffer : m_UniformBuffers)
			uniformBuffer->Destroy();
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
}