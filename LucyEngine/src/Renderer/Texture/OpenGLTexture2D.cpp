#include "OpenGLTexture2D.h"

#include "stb/stb_image.h"
#include "../Renderer.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLTexture2D::OpenGLTexture2D(TextureSpecification& specs)
		: Texture2D(specs)
	{
		//if you want to create a texture buffer, you can leave the path empty and the stbi_load would fail.
		uint8_t* data = nullptr;
		if(specs.Path) {
			data = stbi_load(specs.Path, &m_Width, &m_Height, &m_Channels, 0);

			if (m_Channels == 1)
				specs.Format.Format = GL_RED;
			else if (m_Channels == 3)
				specs.Format.Format = GL_RGB;
			else if (m_Channels == 4)
				specs.Format.Format = GL_RGBA;

			specs.Format.InternalFormat = specs.Format.Format;

			if (!data) LUCY_CRITICAL(std::string("Failed to load a texture. Texture name: ").append(specs.Path));
		}
		
		LUCY_ASSERT(m_Width != 0 && m_Height != 0);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
		Bind();

		if (specs.Parameter.S != 0)		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, specs.Parameter.S);
		if (specs.Parameter.T != 0)		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, specs.Parameter.T);
		if (specs.Parameter.R != 0)		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, specs.Parameter.R);
		if (specs.Parameter.Min != 0)	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, specs.Parameter.Min);
		if (specs.Parameter.Mag != 0)	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, specs.Parameter.Mag);

		!data ? glTexImage2D(GL_TEXTURE_2D, 0, specs.Format.InternalFormat, m_Width, m_Height, 0, specs.Format.Format, (GLenum)specs.PixelType, 0) :
			glTexImage2D(GL_TEXTURE_2D, 0, specs.Format.InternalFormat, m_Width, m_Height, 0, specs.Format.Format, (GLenum)specs.PixelType, data);

		if (specs.GenerateMipmap) glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(data);
		Unbind();
	}

	void OpenGLTexture2D::Bind()
	{
		glBindTexture(GL_TEXTURE_2D, m_Id);
	}

	void OpenGLTexture2D::Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLTexture2D::Destroy()
	{
		glDeleteTextures(1, &m_Id);
	}
}
