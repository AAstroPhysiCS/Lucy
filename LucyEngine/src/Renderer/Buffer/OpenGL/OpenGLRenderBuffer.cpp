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
		Bind();
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_Specs.Attachment, GL_RENDERBUFFER, m_Id);
		Unbind();
	}

	void OpenGLRenderBuffer::Resize(int32_t width, int32_t height) {
		Destroy();

		m_Specs.Width = width;
		m_Specs.Height = height;

		glCreateRenderbuffers(1, &m_Id);
		Bind();
		if (m_Specs.Samples != 0) {
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Specs.Samples, m_Specs.InternalFormat, m_Specs.Width, m_Specs.Height);
		}
		else {
			glRenderbufferStorage(GL_RENDERBUFFER, m_Specs.InternalFormat, m_Specs.Width, m_Specs.Height);
		}
		Unbind();
	}

	void OpenGLRenderBuffer::Destroy() {
		glDeleteRenderbuffers(1, &m_Id);
	}
}
