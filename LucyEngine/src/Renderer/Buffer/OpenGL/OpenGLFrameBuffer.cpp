#include <glad/glad.h>

#include "OpenGLFrameBuffer.h"
#include "../../Texture/Texture.h"
#include "../../Renderer.h"

namespace Lucy {
	
	OpenGLFrameBuffer::OpenGLFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs)
	{
		Renderer::Submit([&]() {
			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < specs.textureSize; i++) {
				RefLucy<Texture2D> texture = Texture2D::Create(specs.textureSpecs[i]);
				texture->Bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->GetID(), 0);
				texture->Unbind();
			}

			LUCY_ASSERT(CheckStatus());
			Unbind();
		});
	}

	void OpenGLFrameBuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
	}

	void OpenGLFrameBuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //binds the default framebuffer (OpenGL only)
	}

	void OpenGLFrameBuffer::Destroy()
	{
		glDeleteFramebuffers(1, &m_Id);
	}

	bool OpenGLFrameBuffer::CheckStatus()
	{
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}

}
