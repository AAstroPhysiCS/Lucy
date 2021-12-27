#include "lypch.h"
#include "OpenGLPipeline.h"
#include "VulkanPipeline.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	RefLucy<Pipeline> Pipeline::Create(PipelineSpecification& specs) {
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
	}

	Pipeline::Pipeline(PipelineSpecification& specs)
		: m_Specs(specs) {
	}
}