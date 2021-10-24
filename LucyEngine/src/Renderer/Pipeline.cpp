#include "Pipeline.h"

namespace Lucy {
	
	RefLucy<Pipeline>& Pipeline::Create(PipelineSpecification& specs)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
		case RenderContextType::OpenGL:
			return CreateRef<Pipeline>(specs);
			break;
		default:
			LUCY_CRITICAL("Other API's not supported!");
			LUCY_ASSERT(false);
			break;
		}
	}

	Pipeline::Pipeline(PipelineSpecification& specs)
	{
		m_Specs = specs;
	}
}
