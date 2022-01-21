#include "lypch.h"
#include "OpenGLRenderPass.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLRenderPass::OpenGLRenderPass(RenderPassSpecification& specs) 
		: RenderPass(specs)
	{
	}
	
	void OpenGLRenderPass::Begin(RenderPassBeginInfo& info) {
		auto& frameBuffer = info.OpenGLFrameBuffer;
		frameBuffer->Bind();
	}
	
	void OpenGLRenderPass::End(RenderPassEndInfo& info) {
		auto& frameBuffer = info.OpenGLFrameBuffer;
		frameBuffer->Unbind();
	}
}
