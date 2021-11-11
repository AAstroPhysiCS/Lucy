#include "OpenGLRenderBuffer.h"
#include "OpenGLFrameBuffer.h"
#include <glad/glad.h>

namespace Lucy {

	OpenGLRenderBuffer::OpenGLRenderBuffer(RenderBufferSpecification& specs)
		: RenderBuffer(specs) {
		glCreateRenderbuffers(1, &m_Id);
		Bind();
		if (specs.Samples != 0) {
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, specs.Samples, specs.InternalFormat, specs.Width, specs.Height);
		}
		else {
			glRenderbufferStorage(GL_RENDERBUFFER, specs.InternalFormat, specs.Width, specs.Height);
		}
		Unbind();
	}

	void OpenGLRenderBuffer::Bind() {
		glBindRenderbuffer(GL_RENDERBUFFER, m_Id);
	}

	void OpenGLRenderBuffer::Unbind() {
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void OpenGLRenderBuffer::AttachToFramebuffer() {
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_Specs.Attachment, GL_RENDERBUFFER, m_Id);
	}
}
