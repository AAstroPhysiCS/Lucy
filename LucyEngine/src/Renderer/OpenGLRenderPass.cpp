#include "lypch.h"
#include "OpenGLRenderPass.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLRenderPass::OpenGLRenderPass(RenderPassSpecification& specs) 
		: RenderPass(specs)
	{
	}
	
	void OpenGLRenderPass::Begin() {
		auto& frameBuffer = m_Specs.FrameBuffer;
		frameBuffer->Bind();

		RefLucy<Pipeline> pipeline = m_Specs.Pipeline;
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
	
	void OpenGLRenderPass::End() {
		auto& frameBuffer = m_Specs.FrameBuffer;

		//reverting the changes back
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glLineWidth(1.0f);
		glDisable(GL_CULL_FACE);
		frameBuffer->Unbind();
	}
}
