#include "OpenGLRenderBuffer.h"
#include "OpenGLFrameBuffer.h"
#include <glad/glad.h>

namespace Lucy {
	
	OpenGLRenderBuffer::OpenGLRenderBuffer(RenderBufferSpecification& specs)
		: RenderBuffer(specs)
	{
		glCreateRenderbuffers(1, &m_Id);
		Bind();
		if (specs.samples != 0) {
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, specs.samples, specs.internalFormat, specs.width, specs.height);
		}
		else {
			glRenderbufferStorage(GL_RENDERBUFFER, specs.internalFormat, specs.width, specs.height);
		}
		Unbind();
	}

	void OpenGLRenderBuffer::Bind()
	{
		glBindRenderbuffer(GL_RENDERBUFFER, m_Id);
	}

	void OpenGLRenderBuffer::Unbind()
	{
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void OpenGLRenderBuffer::AttachToFramebuffer()
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_Specs.attachment, GL_RENDERBUFFER, m_Id);
	}
}
