#include "lypch.h"
#include "OpenGLRenderPass.h"

#include "Memory/Buffer/OpenGL/OpenGLFrameBuffer.h"

namespace Lucy {

	OpenGLRenderPass::OpenGLRenderPass(const RenderPassCreateInfo& createInfo)
		: RenderPass(createInfo) {
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
