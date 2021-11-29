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
			m_Textures.clear();
			
			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < specs.TextureSpecs.size(); i++) {
				auto textureSpec = specs.TextureSpecs[i];
				RefLucy<OpenGLTexture2D> texture = As(Texture2D::Create(textureSpec), OpenGLTexture2D);
				texture->Bind();
				if (textureSpec.Format.Format == GL_DEPTH_COMPONENT || textureSpec.Format.InternalFormat == GL_DEPTH_COMPONENT)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->GetTarget(), texture->GetID(), 0);
				else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + textureSpec.AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);

				if (specs.IsStorage) {
					if (specs.MultiSampled)
						glTexStorage2DMultisample(texture->GetTarget(), textureSpec.Samples, textureSpec.Format.InternalFormat, textureSpec.Width, textureSpec.Height, false);
					else
						glTexStorage2D(texture->GetTarget(), 1, textureSpec.Format.InternalFormat, textureSpec.Width, textureSpec.Height);
				}

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
			} else {
				for (uint32_t i = 0; i < m_Specs.TextureSpecs.size(); i++) {
					if (!specs.TextureSpecs[i].DisableReadWriteBuffer && specs.TextureSpecs[i].Format.Format != GL_DEPTH_COMPONENT)
						glDrawBuffer(GL_COLOR_ATTACHMENT0 + specs.TextureSpecs[i].AttachmentIndex);
				}
			}

			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		//we have to blit it if blit texture is defined
		if (specs.BlittedTextureSpecs.Width != 0 && specs.BlittedTextureSpecs.Height != 0) {
			FrameBufferSpecification blittedSpec;
			blittedSpec.ViewportWidth = specs.ViewportWidth;
			blittedSpec.ViewportHeight = specs.ViewportHeight;
			blittedSpec.MultiSampled = false;
			blittedSpec.TextureSpecs.push_back(specs.BlittedTextureSpecs);
			m_Blitted = FrameBuffer::Create(blittedSpec);
			As(m_Blitted, OpenGLFrameBuffer)->CheckStatus();
		}
	}

	void OpenGLFrameBuffer::Bind() {
		if (m_Specs.ViewportWidth == 0 || m_Specs.ViewportHeight == 0) LUCY_ASSERT(false);
		glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
		glViewport(0, 0, m_Specs.ViewportWidth, m_Specs.ViewportHeight);
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
			m_Textures.clear();

			m_Specs.ViewportWidth = width;
			m_Specs.ViewportHeight = height;

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < m_Specs.TextureSpecs.size(); i++) {
				auto& textureSpec = m_Specs.TextureSpecs[i];
				textureSpec.Width = width;
				textureSpec.Height = height;
				RefLucy<OpenGLTexture2D> texture = As(Texture2D::Create(textureSpec), OpenGLTexture2D);
				texture->Bind();
				if (textureSpec.Format.Format == GL_DEPTH_COMPONENT || textureSpec.Format.InternalFormat == GL_DEPTH_COMPONENT)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->GetTarget(), texture->GetID(), 0);
				else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + textureSpec.AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);

				if (m_Specs.IsStorage) {
					if (m_Specs.MultiSampled)
						glTexStorage2DMultisample(texture->GetTarget(), textureSpec.Samples, textureSpec.Format.InternalFormat, textureSpec.Width, textureSpec.Height, false);
					else
						glTexStorage2D(texture->GetTarget(), 1, textureSpec.Format.InternalFormat, textureSpec.Width, textureSpec.Height);
				}

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
			} else {
				for (uint32_t i = 0; i < m_Specs.TextureSpecs.size(); i++) {
					if (!m_Specs.TextureSpecs[i].DisableReadWriteBuffer && m_Specs.TextureSpecs[i].Format.Format != GL_DEPTH_COMPONENT)
						glDrawBuffer(GL_COLOR_ATTACHMENT0 + m_Specs.TextureSpecs[i].AttachmentIndex);
				}
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