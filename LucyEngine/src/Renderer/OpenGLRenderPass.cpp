#include "lypch.h"
#include "OpenGLRenderPass.h"

#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLRenderPass::OpenGLRenderPass(const RenderPassSpecification& specs)
		: RenderPass(specs) {
	}

	void OpenGLRenderPass::Begin(RenderPassBeginInfo& info) {
		m_BoundedFrameBuffer = info.OpenGLFrameBuffer;
		m_BoundedFrameBuffer->Bind();
	}

	void OpenGLRenderPass::End() {
		m_BoundedFrameBuffer->Unbind();
	}

	void OpenGLRenderPass::Destroy() {
	
	}
	
	void OpenGLRenderPass::Recreate() {
	
	}
}
