#include "lypch.h"
#include "OpenGLImage.h"

#include "../Renderer.h"

#include "glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Lucy {

	uint16_t GetOpenGLType(const ImageCreateInfo& createInfo) {
		return createInfo.Samples != 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	OpenGLImage2D::OpenGLImage2D(const std::string& path, ImageCreateInfo& createInfo)
		: Image2D(path, createInfo) {
		Ref<OpenGLRHIImageDesc> imageDesc = m_CreateInfo.InternalInfo.As<OpenGLRHIImageDesc>();

		uint8_t* data = stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Channels, 0);

		if (m_Channels == 1)
			m_CreateInfo.Format = GL_RED;
		else if (m_Channels == 3)
			m_CreateInfo.Format = GL_RGB;
		else if (m_Channels == 4)
			m_CreateInfo.Format = GL_RGBA;

		imageDesc->InternalFormat = m_CreateInfo.Format;

		if (!data) LUCY_CRITICAL(fmt::format("Failed to load a texture. Texture path: {0}", m_Path));

		stbi_image_free(data);

		glCreateTextures(m_Target, 1, &m_Id);
		Bind();

		SetParameters(imageDesc);
		if (m_CreateInfo.Samples == 0)
			glTexImage2D(m_Target, 0, imageDesc->InternalFormat, m_Width, m_Height, 0, m_CreateInfo.Format, (GLenum)imageDesc->PixelType, data);
		else
			glTexImage2DMultisample(m_Target, m_CreateInfo.Samples, imageDesc->InternalFormat, m_Width, m_Height, true);

		Unbind();
	}

	OpenGLImage2D::OpenGLImage2D(ImageCreateInfo& createInfo)
		: Image2D(createInfo) {
		if (m_Width == 0 && m_Height == 0) LUCY_ASSERT(false);

		Ref<OpenGLRHIImageDesc> imageDesc = m_CreateInfo.InternalInfo.As<OpenGLRHIImageDesc>();

		m_Target = GetOpenGLType(m_CreateInfo);

		glCreateTextures(m_Target, 1, &m_Id);
		Bind();
		SetParameters(imageDesc);

		glTexImage2D(m_Target, 0, imageDesc->InternalFormat, m_Width, m_Height, 0, m_CreateInfo.Format, (GLenum)imageDesc->PixelType, 0);
		if (m_CreateInfo.GenerateMipmap) 
			glGenerateMipmap(m_Target);

		Unbind();
	}

	void OpenGLImage2D::SetParameters(const Ref<OpenGLRHIImageDesc>& imageDesc) {
		if (m_CreateInfo.Parameter.W != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, m_CreateInfo.Parameter.W);
		if (m_CreateInfo.Parameter.V != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, m_CreateInfo.Parameter.V);
		if (m_CreateInfo.Parameter.U != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_R, m_CreateInfo.Parameter.U);
		if (m_CreateInfo.Parameter.Min != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, m_CreateInfo.Parameter.Min);
		if (m_CreateInfo.Parameter.Mag != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, m_CreateInfo.Parameter.Mag);
	}

	void OpenGLImage2D::Bind() {
		glBindTexture(m_Target, m_Id);
	}

	void OpenGLImage2D::Unbind() {
		glBindTexture(m_Target, 0);
	}

	void OpenGLImage2D::Destroy() {
		glDeleteTextures(1, &m_Id);
	}
}