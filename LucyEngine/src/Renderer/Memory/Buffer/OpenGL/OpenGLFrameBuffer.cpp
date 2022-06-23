#include "lypch.h"

#include "OpenGLFrameBuffer.h"
#include "OpenGLRenderBuffer.h"

#include "Renderer/Image/OpenGLImage.h"
#include "Renderer/Renderer.h"

#include <glad/glad.h>

namespace Lucy {

	OpenGLFrameBuffer::OpenGLFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs) {
		Ref<OpenGLRHIFrameBufferDesc> frameBufferDesc = specs.InternalInfo.As<OpenGLRHIFrameBufferDesc>();

		Renderer::Enqueue([=]() {
			m_Textures.clear();
			
			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < frameBufferDesc->TextureSpecs.size(); i++) {
				auto textureSpec = frameBufferDesc->TextureSpecs[i];
				Ref<OpenGLRHIImageDesc> imageDesc = textureSpec.InternalInfo.As<OpenGLRHIImageDesc>();
				Ref<OpenGLImage2D> texture = Image2D::Create(textureSpec).As<OpenGLImage2D>();
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
				Ref<OpenGLRenderBuffer>& renderBuffer = frameBufferDesc->RenderBuffer.As<OpenGLRenderBuffer>();
				renderBuffer->AttachToFramebuffer();
			}

			if (frameBufferDesc->DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			} else {
				for (uint32_t i = 0; i < frameBufferDesc->TextureSpecs.size(); i++) {
					Ref<OpenGLRHIImageDesc> imageDesc = frameBufferDesc->TextureSpecs[i].InternalInfo.As<OpenGLRHIImageDesc>();
					if (!imageDesc->DisableReadWriteBuffer && frameBufferDesc->TextureSpecs[i].Format != GL_DEPTH_COMPONENT)
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
			frameBufferDesc->TextureSpecs.push_back(frameBufferDesc->BlittedTextureSpecs);
			m_Blitted = FrameBuffer::Create(blittedSpec).As<OpenGLFrameBuffer>();
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

		for (Ref<Image2D> texture : m_Textures) {
			texture->Destroy();
		}
	}

	void OpenGLFrameBuffer::Blit() {
		Ref<OpenGLRHIFrameBufferDesc> frameBufferDesc = m_Specs.InternalInfo.As<OpenGLRHIFrameBufferDesc>();
		glBlitNamedFramebuffer(m_Id, m_Blitted->GetID(), 0, 0, frameBufferDesc->RenderBuffer->GetWidth(), frameBufferDesc->RenderBuffer->GetHeight(), 
							   0, 0, frameBufferDesc->RenderBuffer->GetWidth(), frameBufferDesc->RenderBuffer->GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	void OpenGLFrameBuffer::Resize(int32_t width, int32_t height) {
		Ref<OpenGLRHIFrameBufferDesc> frameBufferDesc = m_Specs.InternalInfo.As<OpenGLRHIFrameBufferDesc>();

		Renderer::Enqueue([=]() {
			Destroy();
			m_Textures.clear();

			m_Specs.Width = width;
			m_Specs.Height = height;

			glCreateFramebuffers(1, &m_Id);
			Bind();

			for (uint32_t i = 0; i < frameBufferDesc->TextureSpecs.size(); i++) {
				auto& textureSpec = frameBufferDesc->TextureSpecs[i];
				textureSpec.Width = width;
				textureSpec.Height = height;
				Ref<OpenGLRHIImageDesc> imageDesc = textureSpec.InternalInfo.As<OpenGLRHIImageDesc>();
				Ref<OpenGLImage2D> texture = Image2D::Create(textureSpec).As<OpenGLImage2D>();
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
				Ref<OpenGLRenderBuffer> renderBuffer = frameBufferDesc->RenderBuffer.As<OpenGLRenderBuffer>();
				renderBuffer->Resize(width, height);
				renderBuffer->AttachToFramebuffer();
			}

			if (frameBufferDesc->DisableReadWriteBuffer) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			} else {
				for (uint32_t i = 0; i < frameBufferDesc->TextureSpecs.size(); i++) {
					Ref<OpenGLRHIImageDesc> imageDesc = frameBufferDesc->TextureSpecs[i].InternalInfo.As<OpenGLRHIImageDesc>();
					if (!imageDesc->DisableReadWriteBuffer && frameBufferDesc->TextureSpecs[i].Format != GL_DEPTH_COMPONENT)
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