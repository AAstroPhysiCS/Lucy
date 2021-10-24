#include "OpenGLFrameBuffer.h"
#include "OpenGLRenderBuffer.h"
#include "../../Texture/Texture.h"
#include "../../Renderer.h"

#include <glad/glad.h>

namespace Lucy {

	OpenGLFrameBuffer::OpenGLFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs)
	{
		Renderer::Submit([=]() {

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < specs.TextureSpecs.size(); i++) {
				auto textureSpec = specs.TextureSpecs[i];
				RefLucy<Texture2D> texture = Texture2D::Create(textureSpec);
				texture->Bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + textureSpec.AttachmentIndex, GL_TEXTURE_2D, texture->GetID(), 0);
				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (specs.RenderBuffer) {
				RefLucy<OpenGLRenderBuffer>& renderBuffer = As(specs.RenderBuffer, OpenGLRenderBuffer);
				renderBuffer->Bind();
				renderBuffer->AttachToFramebuffer();
				renderBuffer->Unbind();
			}
			
			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		//we have to blit it if blit texture is defined
		if (specs.BlittedTextureSpecs.Width != 0 && specs.BlittedTextureSpecs.Height != 0) {
			FrameBufferSpecification blittedSpec;
			blittedSpec.MultiSampled = false;
			blittedSpec.TextureSpecs.push_back(specs.BlittedTextureSpecs);
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
		glBlitNamedFramebuffer(m_Id, m_Blitted->GetID(), 0, 0, m_Specs.RenderBuffer->GetWidth(), m_Specs.RenderBuffer->GetHeight(), 0, 0, m_Specs.RenderBuffer->GetWidth(), m_Specs.RenderBuffer->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	bool OpenGLFrameBuffer::CheckStatus()
	{
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
}
