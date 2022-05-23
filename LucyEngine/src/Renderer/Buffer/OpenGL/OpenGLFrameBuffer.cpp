#include "lypch.h"

#include "OpenGLFrameBuffer.h"
#include "OpenGLRenderBuffer.h"
#include "../../Image/OpenGLImage.h"
#include "../../Renderer.h"

#include <glad/glad.h>

namespace Lucy {

	OpenGLFrameBuffer::OpenGLFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs) {
		RefLucy<OpenGLRHIFrameBufferDesc> frameBufferDesc = As(specs.InternalInfo, OpenGLRHIFrameBufferDesc);

		Renderer::Enqueue([=]() {
			m_Textures.clear();
			
			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < specs.TextureSpecs.size(); i++) {
				auto textureSpec = specs.TextureSpecs[i];
				RefLucy<OpenGLRHIImageDesc> imageDesc = As(textureSpec.InternalInfo, OpenGLRHIImageDesc);
				RefLucy<OpenGLImage2D> texture = As(Image2D::Create(textureSpec), OpenGLImage2D);
				texture->Bind();
				if (textureSpec.Format == GL_DEPTH_COMPONENT || imageDesc->InternalFormat == GL_DEPTH_COMPONENT)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->GetTarget(), texture->GetID(), 0);
				else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);

				if (frameBufferDesc->IsStorage) {
					if (specs.MultiSampled)
						glTexStorage2DMultisample(texture->GetTarget(), textureSpec.Samples, imageDesc->InternalFormat, textureSpec.Width, textureSpec.Height, false);
					else
						glTexStorage2D(texture->GetTarget(), 1, imageDesc->InternalFormat, textureSpec.Width, textureSpec.Height);
				}

				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (frameBufferDesc->RenderBuffer) {
				RefLucy<OpenGLRenderBuffer>& renderBuffer = As(frameBufferDesc->RenderBuffer, OpenGLRenderBuffer);
				renderBuffer->AttachToFramebuffer();
			}

			if (frameBufferDesc->DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			} else {
				for (uint32_t i = 0; i < m_Specs.TextureSpecs.size(); i++) {
					RefLucy<OpenGLRHIImageDesc> imageDesc = As(m_Specs.TextureSpecs[i].InternalInfo, OpenGLRHIImageDesc);
					if (!imageDesc->DisableReadWriteBuffer && specs.TextureSpecs[i].Format != GL_DEPTH_COMPONENT)
						glDrawBuffer(GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex);
				}
			}

			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		//we have to blit it if blit texture is defined
		if (frameBufferDesc->BlittedTextureSpecs.Width != 0 && frameBufferDesc->BlittedTextureSpecs.Height != 0) {
			FrameBufferSpecification blittedSpec;
			blittedSpec.Width = specs.Width;
			blittedSpec.Height = specs.Height;
			blittedSpec.MultiSampled = false;
			blittedSpec.TextureSpecs.push_back(frameBufferDesc->BlittedTextureSpecs);
			m_Blitted = As(FrameBuffer::Create(blittedSpec), OpenGLFrameBuffer);
			m_Blitted->CheckStatus();
		}
	}

	void OpenGLFrameBuffer::Bind() {
		if (m_Specs.Width == 0 || m_Specs.Height == 0) LUCY_ASSERT(false);
		glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
		glViewport(0, 0, m_Specs.Width, m_Specs.Height);
	}

	void OpenGLFrameBuffer::Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //binds actually the default framebuffer
	}

	void OpenGLFrameBuffer::Destroy() {
		glDeleteFramebuffers(1, &m_Id);

		for (RefLucy<Image2D> texture : m_Textures) {
			texture->Destroy();
		}
	}

	void OpenGLFrameBuffer::Blit() {
		RefLucy<OpenGLRHIFrameBufferDesc> frameBufferDesc = As(m_Specs.InternalInfo, OpenGLRHIFrameBufferDesc);
		glBlitNamedFramebuffer(m_Id, m_Blitted->GetID(), 0, 0, frameBufferDesc->RenderBuffer->GetWidth(), frameBufferDesc->RenderBuffer->GetHeight(), 
							   0, 0, frameBufferDesc->RenderBuffer->GetWidth(), frameBufferDesc->RenderBuffer->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	void OpenGLFrameBuffer::Resize(int32_t width, int32_t height) {
		RefLucy<OpenGLRHIFrameBufferDesc> frameBufferDesc = As(m_Specs.InternalInfo, OpenGLRHIFrameBufferDesc);

		Renderer::Enqueue([=]() {
			Destroy();
			m_Textures.clear();

			m_Specs.Width = width;
			m_Specs.Height = height;

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < m_Specs.TextureSpecs.size(); i++) {
				auto& textureSpec = m_Specs.TextureSpecs[i];
				textureSpec.Width = width;
				textureSpec.Height = height;
				RefLucy<OpenGLRHIImageDesc> imageDesc = As(textureSpec.InternalInfo, OpenGLRHIImageDesc);
				RefLucy<OpenGLImage2D> texture = As(Image2D::Create(textureSpec), OpenGLImage2D);
				texture->Bind();
				if (textureSpec.Format == GL_DEPTH_COMPONENT || imageDesc->InternalFormat == GL_DEPTH_COMPONENT)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->GetTarget(), texture->GetID(), 0);
				else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);

				if (frameBufferDesc->IsStorage) {
					if (m_Specs.MultiSampled)
						glTexStorage2DMultisample(texture->GetTarget(), textureSpec.Samples, imageDesc->InternalFormat, textureSpec.Width, textureSpec.Height, false);
					else
						glTexStorage2D(texture->GetTarget(), 1, imageDesc->InternalFormat, textureSpec.Width, textureSpec.Height);
				}

				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (frameBufferDesc->RenderBuffer) {
				RefLucy<OpenGLRenderBuffer> renderBuffer = As(frameBufferDesc->RenderBuffer, OpenGLRenderBuffer);
				renderBuffer->Resize(width, height);
				renderBuffer->AttachToFramebuffer();
			}

			if (frameBufferDesc->DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			} else {
				for (uint32_t i = 0; i < m_Specs.TextureSpecs.size(); i++) {
					RefLucy<OpenGLRHIImageDesc> imageDesc = As(m_Specs.TextureSpecs[i].InternalInfo, OpenGLRHIImageDesc);
					if (!imageDesc->DisableReadWriteBuffer && m_Specs.TextureSpecs[i].Format != GL_DEPTH_COMPONENT)
						glDrawBuffer(GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex);
				}
			}

			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		if (frameBufferDesc->BlittedTextureSpecs.Width != 0 && frameBufferDesc->BlittedTextureSpecs.Height != 0) {
			m_Blitted->Resize(width, height);
		}
	}

	bool OpenGLFrameBuffer::CheckStatus() {
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
}