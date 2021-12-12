#include "lypch.h"
#include "OpenGLPipeline.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	RefLucy<Pipeline> Pipeline::Create(PipelineSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLPipeline>(specs);
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