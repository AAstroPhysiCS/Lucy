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

		auto [width, height] = Renderer::GetViewportSize();
		glViewport(0, 0, width, height);

		RefLucy<Pipeline> pipeline = renderPass->GetPipeline();
		Rasterization rasterization = pipeline->GetRasterization();
		glPolygonMode(GL_FRONT_AND_BACK, rasterization.PolygonMode);
		glLineWidth(rasterization.LineWidth);
		if (rasterization.DisableBackCulling)
			glDisable(GL_CULL_FACE);
	}

	void RenderPass::End(RefLucy<RenderPass>& renderPass) {
		auto& frameBuffer = renderPass->m_Specs.FrameBuffer;
		frameBuffer->Unbind();

		//reverting the changes back
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glLineWidth(1.0f);
		glEnable(GL_CULL_FACE);
	}
}