#include "lypch.h"

#include "OpenGLFrameBuffer.h"
#include "OpenGLRenderBuffer.h"
#include "../../Texture/OpenGLTexture2D.h"
#include "../../Renderer.h"

#include <glad/glad.h>

namespace Lucy {

	OpenGLFrameBuffer::OpenGLFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs) {
		Renderer::Submit([=]() {

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < specs.TextureSpecs.size(); i++) {
				auto textureSpec = specs.TextureSpecs[i];
				RefLucy<OpenGLTexture2D> texture = As(Texture2D::Create(textureSpec), OpenGLTexture2D);
				texture->Bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + textureSpec.AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);
				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (specs.RenderBuffer) {
				RefLucy<OpenGLRenderBuffer>& renderBuffer = As(specs.RenderBuffer, OpenGLRenderBuffer);
				renderBuffer->AttachToFramebuffer();
			}

			if (specs.DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
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
			As(m_Blitted, OpenGLFrameBuffer)->CheckStatus();
		}
	}

	void OpenGLFrameBuffer::Bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
		
		auto [w, h] = Renderer::GetViewportSize();
		glViewport(0, 0, w, h);
	}

	void OpenGLFrameBuffer::Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //binds actually the default framebuffer (OpenGL only)
	}

	void OpenGLFrameBuffer::Destroy() {
		glDeleteFramebuffers(1, &m_Id);

		for (RefLucy<Texture2D> texture : m_Textures) {
			texture->Destroy();
		}
	}

	void OpenGLFrameBuffer::Blit() {
		glBlitNamedFramebuffer(m_Id, m_Blitted->GetID(), 0, 0, m_Specs.RenderBuffer->GetWidth(), m_Specs.RenderBuffer->GetHeight(), 0, 0, m_Specs.RenderBuffer->GetWidth(), m_Specs.RenderBuffer->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	void OpenGLFrameBuffer::Resize(int32_t width, int32_t height) {

		Renderer::Submit([=]() {
			Destroy();

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < m_Specs.TextureSpecs.size(); i++) {
				auto& textureSpec = m_Specs.TextureSpecs[i];
				textureSpec.Width = width;
				textureSpec.Height = height;
				RefLucy<OpenGLTexture2D> texture = As(Texture2D::Create(textureSpec), OpenGLTexture2D);
				texture->Bind();
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + textureSpec.AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);
				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (m_Specs.RenderBuffer) {
				RefLucy<OpenGLRenderBuffer> renderBuffer = As(m_Specs.RenderBuffer, OpenGLRenderBuffer);
				renderBuffer->Resize(width, height);
				renderBuffer->AttachToFramebuffer();
			}

			if (m_Specs.DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}

			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		if (m_Specs.BlittedTextureSpecs.Width != 0 && m_Specs.BlittedTextureSpecs.Height != 0) {
			m_Blitted->Resize(width, height);
		}
	}

	bool OpenGLFrameBuffer::CheckStatus() {
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
}