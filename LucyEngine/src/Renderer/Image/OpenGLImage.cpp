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

	OpenGLImage2D::OpenGLImage2D(ImageSpecification& specs)
		: Image2D(specs) {
		RefLucy<OpenGLRHIImageDesc> imageDesc = As(specs.InternalInfo, OpenGLRHIImageDesc);
		m_Slot = imageDesc->Slot;

		//if you want to create a texture buffer, you can leave the path empty and the stbi_load would fail.
		uint8_t* data = nullptr;
		if (specs.Path) {
			data = stbi_load(specs.Path, &m_Width, &m_Height, &m_Channels, 0);

			if (m_Channels == 1)
				specs.Format = GL_RED;
			else if (m_Channels == 3)
				specs.Format = GL_RGB;
			else if (m_Channels == 4)
				specs.Format = GL_RGBA;

			imageDesc->InternalFormat = specs.Format;

			if (!data) LUCY_CRITICAL(fmt::format("Failed to load a texture. Texture path: {0}", specs.Path));
		} else {
			m_Width = specs.Width;
			m_Height = specs.Height;
		}

		m_Target = GetOpenGLType(specs);

		if (m_Width == 0 && m_Height == 0) LUCY_ASSERT(false);

		glCreateTextures(m_Target, 1, &m_Id);
		Bind();

		if (specs.Parameter.W != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, specs.Parameter.W);
		if (specs.Parameter.V != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, specs.Parameter.V);
		if (specs.Parameter.U != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_WRAP_R, specs.Parameter.U);
		if (specs.Parameter.Min != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, specs.Parameter.Min);
		if (specs.Parameter.Mag != 0 && m_Target != GL_TEXTURE_2D_MULTISAMPLE)	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, specs.Parameter.Mag);

		if (specs.Samples == 0) {
			!data ? glTexImage2D(m_Target, 0, imageDesc->InternalFormat, m_Width, m_Height, 0, specs.Format, (GLenum)imageDesc->PixelType, 0) :
				glTexImage2D(m_Target, 0, imageDesc->InternalFormat, m_Width, m_Height, 0, specs.Format, (GLenum)imageDesc->PixelType, data);
		} else {
			glTexImage2DMultisample(m_Target, specs.Samples, imageDesc->InternalFormat, m_Width, m_Height, true);
		}

		if (specs.GenerateMipmap) glGenerateMipmap(m_Target);

		stbi_image_free(data);
		Unbind();
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