#include "RenderPass.h"
#include "Renderer.h"

namespace Lucy {

	RenderPass::RenderPass(RenderPassSpecification& specs)
		: m_Specs(specs)
	{
	}

	RefLucy<RenderPass> RenderPass::Create(RenderPassSpecification& specs)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
		case RenderContextType::OpenGL:
			return CreateRef<RenderPass>(specs);
			break;
		default:
			LUCY_CRITICAL("Other API's are not supported!");
			LUCY_ASSERT(false);
			break;
		}
	}

	void RenderPass::Begin(RefLucy<RenderPass>& renderPass)
	{
		auto& frameBuffer = renderPass->m_Specs.FrameBuffer;
		frameBuffer->Bind();
	}

	void RenderPass::End(RefLucy<RenderPass>& renderPass)
	{
		auto& frameBuffer = renderPass->m_Specs.FrameBuffer;
		frameBuffer->Unbind();
	}
}