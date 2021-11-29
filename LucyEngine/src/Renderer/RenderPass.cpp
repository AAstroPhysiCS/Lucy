#include "lypch.h"

#include "RenderPass.h"
#include "Renderer.h"

namespace Lucy {

	RenderPass::RenderPass(RenderPassSpecification& specs)
		: m_Specs(specs) {
	}

	RefLucy<RenderPass> RenderPass::Create(RenderPassSpecification& specs) {
		switch (Renderer::GetCurrentRenderAPI()) {
			case RenderAPI::OpenGL:
				return CreateRef<RenderPass>(specs);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
	}

	void RenderPass::Begin(RefLucy<RenderPass>& renderPass) {
		auto& frameBuffer = renderPass->m_Specs.FrameBuffer;
		frameBuffer->Bind();

		RefLucy<Pipeline> pipeline = renderPass->GetPipeline();
		Rasterization rasterization = pipeline->GetRasterization();
		glPolygonMode(GL_FRONT_AND_BACK, rasterization.PolygonMode);
		glLineWidth(rasterization.LineWidth);
		if (rasterization.DisableBackCulling)
			glDisable(GL_CULL_FACE);
		if (rasterization.CullingMode != 0) {
			glEnable(GL_CULL_FACE);
			glCullFace(rasterization.CullingMode);
		}
	}

	void RenderPass::End(RefLucy<RenderPass>& renderPass) {
		auto& frameBuffer = renderPass->m_Specs.FrameBuffer;

		//reverting the changes back
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glLineWidth(1.0f);
		glDisable(GL_CULL_FACE);
		frameBuffer->Unbind();
	}
}