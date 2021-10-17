#include <glad/glad.h>

#include "OpenGLFrameBuffer.h"
#include "OpenGLRenderBuffer.h"
#include "../../Texture/Texture.h"
#include "../../Renderer.h"

namespace Lucy {

	OpenGLFrameBuffer::OpenGLFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs)
	{
		Renderer::Submit([=]() {

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < specs.textureSpecs.size(); i++) {
				auto textureSpec = specs.textureSpecs[i];
				RefLucy<Texture2D> texture = Texture2D::Create(textureSpec);
				texture->Bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + textureSpec.attachmentIndex, GL_TEXTURE_2D, texture->GetID(), 0);
				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (specs.renderBuffer) {
				RefLucy<OpenGLRenderBuffer>& renderBuffer = std::static_pointer_cast<OpenGLRenderBuffer>(specs.renderBuffer);
				renderBuffer->Bind();
				renderBuffer->AttachToFramebuffer();
				renderBuffer->Unbind();
			}
			
			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		//we have to blit it if blit texture is defined
		if (specs.blittedTextureSpecs.width != 0 && specs.blittedTextureSpecs.height != 0) {
			FrameBufferSpecification blittedSpec;
			blittedSpec.multiSampled = false;
			blittedSpec.textureSpecs.push_back(specs.blittedTextureSpecs);
			m_Blitted = FrameBuffer::Create(blittedSpec);
		}
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

	void OpenGLFrameBuffer::Blit()
	{
		glBlitNamedFramebuffer(m_Id, m_Blitted->GetID(), 0, 0, m_Specs.renderBuffer->GetWidth(), m_Specs.renderBuffer->GetHeight(), 0, 0, m_Specs.renderBuffer->GetWidth(), m_Specs.renderBuffer->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	bool OpenGLFrameBuffer::CheckStatus()
	{
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
}
