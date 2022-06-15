#include "lypch.h"
#include "OpenGLImage.h"

#include "../Renderer.h"

#include "glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Lucy {

	uint16_t GetOpenGLType(const ImageSpecification& specs) {
		return specs.Samples != 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	OpenGLImage2D::OpenGLImage2D(const std::string& path, ImageSpecification& specs)
		: Image2D(path, specs) {
		RefLucy<OpenGLRHIImageDesc> imageDesc = As(m_Specs.InternalInfo, OpenGLRHIImageDesc);
		m_Slot = imageDesc->Slot;

		uint8_t* data = stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Channels, 0);

		if (m_Channels == 1)
			m_Specs.Format = GL_RED;
		else if (m_Channels == 3)
			m_Specs.Format = GL_RGB;
		else if (m_Channels == 4)
			m_Specs.Format = GL_RGBA;

		imageDesc->InternalFormat = m_Specs.Format;

		if (!data) LUCY_CRITICAL(fmt::format("Failed to load a texture. Texture path: {0}", m_Path));

		stbi_image_free(data);

		glCreateTextures(m_Target, 1, &m_Id);
		Bind();

		SetParameters(imageDesc);
		if (m_Specs.Samples == 0)
			glTexImage2D(m_Target, 0, imageDesc->InternalFormat, m_Width, m_Height, 0, m_Specs.Format, (GLenum)imageDesc->PixelType, data);
		else
			glTexImage2DMultisample(m_Target, m_Specs.Samples, imageDesc->InternalFormat, m_Width, m_Height, true);

		Unbind();
	}

	OpenGLImage2D::OpenGLImage2D(ImageSpecification& specs)
		: Image2D(specs) {
		if (m_Width == 0 && m_Height == 0) LUCY_ASSERT(false);

		RefLucy<OpenGLRHIImageDesc> imageDesc = As(m_Specs.InternalInfo, OpenGLRHIImageDesc);

		m_Slot = imageDesc->Slot;
		m_Target = GetOpenGLType(m_Specs);

		glCreateTextures(m_Target, 1, &m_Id);
		Bind();
		SetParameters(imageDesc);

		glTexImage2D(m_Target, 0, imageDesc->InternalFormat, m_Width, m_Height, 0, m_Specs.Format, (GLenum)imageDesc->PixelType, 0);
		if (m_Specs.GenerateMipmap) glGenerateMipmap(m_Target);

		Unbind();
	}

	void OpenGLImage2D::SetParameters(const RefLucy<OpenGLRHIImageDesc>& imageDesc) {
		if (m_Specs.Parameter.W != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, m_Specs.Parameter.W);
		if (m_Specs.Parameter.V != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, m_Specs.Parameter.V);
		if (m_Specs.Parameter.U != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_R, m_Specs.Parameter.U);
		if (m_Specs.Parameter.Min != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, m_Specs.Parameter.Min);
		if (m_Specs.Parameter.Mag != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, m_Specs.Parameter.Mag);
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