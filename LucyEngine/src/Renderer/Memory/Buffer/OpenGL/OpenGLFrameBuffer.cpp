#include "lypch.h"

#include "OpenGLFrameBuffer.h"
#include "OpenGLRenderBuffer.h"

#include "Renderer/Renderer.h"

#include <glad/glad.h>

namespace Lucy {

	OpenGLFrameBuffer::OpenGLFrameBuffer(FrameBufferCreateInfo& createInfo)
		: FrameBuffer(createInfo) {
		Ref<OpenGLRHIFrameBufferDesc> frameBufferDesc = createInfo.InternalInfo;

		Renderer::Enqueue([=]() {
			m_Textures.clear();
			
			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < frameBufferDesc->TextureCreateInfos.size(); i++) {
				auto textureCreateInfo = frameBufferDesc->TextureCreateInfos[i];
				Ref<OpenGLRHIImageDesc> imageDesc = textureCreateInfo.InternalInfo.As<OpenGLRHIImageDesc>();
				Ref<OpenGLImage2D> texture = Image2D::Create(textureCreateInfo).As<OpenGLImage2D>();
				texture->Bind();
				if (textureCreateInfo.Format == GL_DEPTH_COMPONENT || imageDesc->InternalFormat == GL_DEPTH_COMPONENT)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->GetTarget(), texture->GetID(), 0);
				else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);

				if (frameBufferDesc->IsStorage) {
					if (createInfo.MultiSampled)
						glTexStorage2DMultisample(texture->GetTarget(), textureCreateInfo.Samples, imageDesc->InternalFormat, textureCreateInfo.Width, textureCreateInfo.Height, false);
					else
						glTexStorage2D(texture->GetTarget(), 1, imageDesc->InternalFormat, textureCreateInfo.Width, textureCreateInfo.Height);
				}

				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (frameBufferDesc->RenderBuffer) {
				Ref<OpenGLRenderBuffer>& renderBuffer = frameBufferDesc->RenderBuffer.As<OpenGLRenderBuffer>();
				renderBuffer->AttachToFramebuffer();
			}

			if (frameBufferDesc->DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			} else {
				for (uint32_t i = 0; i < frameBufferDesc->TextureCreateInfos.size(); i++) {
					Ref<OpenGLRHIImageDesc> imageDesc = frameBufferDesc->TextureCreateInfos[i].InternalInfo.As<OpenGLRHIImageDesc>();
					if (!imageDesc->DisableReadWriteBuffer && frameBufferDesc->TextureCreateInfos[i].Format != GL_DEPTH_COMPONENT)
						glDrawBuffer(GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex);
				}
			}

			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		//we have to blit it if blit texture is defined
		if (frameBufferDesc->BlittedTextureCreateInfo.Width != 0 && frameBufferDesc->BlittedTextureCreateInfo.Height != 0) {
			FrameBufferCreateInfo blittedCreateInfo;
			blittedCreateInfo.Width = createInfo.Width;
			blittedCreateInfo.Height = createInfo.Height;
			blittedCreateInfo.MultiSampled = false;
			frameBufferDesc->TextureCreateInfos.push_back(frameBufferDesc->BlittedTextureCreateInfo);
			m_Blitted = FrameBuffer::Create(blittedCreateInfo).As<OpenGLFrameBuffer>();
			m_Blitted->CheckStatus();
		}
	}

	void OpenGLFrameBuffer::Bind() {
		if (m_CreateInfo.Width == 0 || m_CreateInfo.Height == 0) LUCY_ASSERT(false);
		glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
		glViewport(0, 0, m_CreateInfo.Width, m_CreateInfo.Height);
	}

	void OpenGLFrameBuffer::Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //binds actually the default framebuffer
	}

	void OpenGLFrameBuffer::Destroy() {
		glDeleteFramebuffers(1, &m_Id);

		for (Ref<Image2D> texture : m_Textures) {
			texture->Destroy();
		}
	}

	void OpenGLFrameBuffer::Blit() {
		Ref<OpenGLRHIFrameBufferDesc> frameBufferDesc = m_CreateInfo.InternalInfo;
		glBlitNamedFramebuffer(m_Id, m_Blitted->GetID(), 0, 0, frameBufferDesc->RenderBuffer->GetWidth(), frameBufferDesc->RenderBuffer->GetHeight(), 
							   0, 0, frameBufferDesc->RenderBuffer->GetWidth(), frameBufferDesc->RenderBuffer->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	void OpenGLFrameBuffer::Resize(int32_t width, int32_t height) {
		Ref<OpenGLRHIFrameBufferDesc> frameBufferDesc = m_CreateInfo.InternalInfo;

		Renderer::Enqueue([=]() {
			Destroy();
			m_Textures.clear();

			m_CreateInfo.Width = width;
			m_CreateInfo.Height = height;

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < frameBufferDesc->TextureCreateInfos.size(); i++) {
				auto& textureCreateInfo = frameBufferDesc->TextureCreateInfos[i];
				textureCreateInfo.Width = width;
				textureCreateInfo.Height = height;
				Ref<OpenGLRHIImageDesc> imageDesc = textureCreateInfo.InternalInfo.As<OpenGLRHIImageDesc>();
				Ref<OpenGLImage2D> texture = Image2D::Create(textureCreateInfo).As<OpenGLImage2D>();
				texture->Bind();
				if (textureCreateInfo.Format == GL_DEPTH_COMPONENT || imageDesc->InternalFormat == GL_DEPTH_COMPONENT)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->GetTarget(), texture->GetID(), 0);
				else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex, texture->GetTarget(), texture->GetID(), 0);

				if (frameBufferDesc->IsStorage) {
					if (m_CreateInfo.MultiSampled)
						glTexStorage2DMultisample(texture->GetTarget(), textureCreateInfo.Samples, imageDesc->InternalFormat, textureCreateInfo.Width, textureCreateInfo.Height, false);
					else
						glTexStorage2D(texture->GetTarget(), 1, imageDesc->InternalFormat, textureCreateInfo.Width, textureCreateInfo.Height);
				}

				texture->Unbind();
				m_Textures.push_back(texture);
			}

			if (frameBufferDesc->RenderBuffer) {
				Ref<OpenGLRenderBuffer> renderBuffer = frameBufferDesc->RenderBuffer.As<OpenGLRenderBuffer>();
				renderBuffer->Resize(width, height);
				renderBuffer->AttachToFramebuffer();
			}

			if (frameBufferDesc->DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			} else {
				for (uint32_t i = 0; i < frameBufferDesc->TextureCreateInfos.size(); i++) {
					Ref<OpenGLRHIImageDesc> imageDesc = frameBufferDesc->TextureCreateInfos[i].InternalInfo.As<OpenGLRHIImageDesc>();
					if (!imageDesc->DisableReadWriteBuffer && frameBufferDesc->TextureCreateInfos[i].Format != GL_DEPTH_COMPONENT)
						glDrawBuffer(GL_COLOR_ATTACHMENT0 + imageDesc->AttachmentIndex);
				}
			}

			LUCY_ASSERT(CheckStatus());
			Unbind();
		});

		if (frameBufferDesc->BlittedTextureCreateInfo.Width != 0 && frameBufferDesc->BlittedTextureCreateInfo.Height != 0) {
			m_Blitted->Resize(width, height);
		}
	}

	bool OpenGLFrameBuffer::CheckStatus() {
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
}