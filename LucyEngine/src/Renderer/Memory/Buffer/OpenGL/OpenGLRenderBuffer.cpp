#include "lypch.h"

#include "OpenGLRenderBuffer.h"
#include <glad/glad.h>

namespace Lucy {

	OpenGLRenderBuffer::OpenGLRenderBuffer(const RenderBufferCreateInfo& createInfo)
		: RenderBuffer(createInfo) {
		glCreateRenderbuffers(1, &m_Id);
		Bind();
		if (createInfo.Samples != 0) {
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, createInfo.Samples, createInfo.InternalFormat, createInfo.Width, createInfo.Height);
		}
		else {
			glRenderbufferStorage(GL_RENDERBUFFER, createInfo.InternalFormat, createInfo.Width, createInfo.Height);
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
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_CreateInfo.Attachment, GL_RENDERBUFFER, m_Id);
		Unbind();
	}

	void OpenGLRenderBuffer::Resize(int32_t width, int32_t height) {
		Destroy();

		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		glCreateRenderbuffers(1, &m_Id);
		Bind();
		if (m_CreateInfo.Samples != 0) {
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_CreateInfo.Samples, m_CreateInfo.InternalFormat, m_CreateInfo.Width, m_CreateInfo.Height);
		}
		else {
			glRenderbufferStorage(GL_RENDERBUFFER, m_CreateInfo.InternalFormat, m_CreateInfo.Width, m_CreateInfo.Height);
		}
		Unbind();
	}

	void OpenGLRenderBuffer::Destroy() {
		glDeleteRenderbuffers(1, &m_Id);
	}
}