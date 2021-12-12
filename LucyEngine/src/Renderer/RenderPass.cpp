#include "lypch.h"

#include "RenderPass.h"
#include "OpenGLRenderPass.h"
#include "Renderer.h"

namespace Lucy {

	RenderPass::RenderPass(RenderPassSpecification& specs)
		: m_Specs(specs) {
	}

	RefLucy<RenderPass> RenderPass::Create(RenderPassSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLRenderPass>(specs);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
	}
}