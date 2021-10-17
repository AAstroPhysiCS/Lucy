#include "OpenGLTexture2D.h"

#include "stb/stb_image.h"

#include "../Renderer.h"

namespace Lucy {

	OpenGLTexture2D::OpenGLTexture2D(TextureSpecification& specs)
		: Texture2D(specs)
	{
		//if you want to create a texture buffer, you can leave the path empty and the stbi_load would fail.
		uint8_t* data = nullptr;
		if(specs.path) {
			data = stbi_load(specs.path, &m_Width, &m_Height, &m_Channels, 0);

			if (m_Channels == 1)
				specs.format.format = GL_RED;
			else if (m_Channels == 3)
				specs.format.format = GL_RGB;
			else if (m_Channels == 4)
				specs.format.format = GL_RGBA;

			specs.format.internalFormat = specs.format.format;

			if (!data) LUCY_CRITICAL(std::string("Failed to load a texture. Texture name: ").append(specs.path));
		}
		
		LUCY_ASSERT(m_Width != 0 && m_Height != 0);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
		Bind();

		if (specs.parameter.S != 0)		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, specs.parameter.S);
		if (specs.parameter.T != 0)		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, specs.parameter.T);
		if (specs.parameter.R != 0)		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, specs.parameter.R);
		if (specs.parameter.min != 0)	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, specs.parameter.min);
		if (specs.parameter.mag != 0)	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, specs.parameter.mag);

		!data ? glTexImage2D(GL_TEXTURE_2D, 0, specs.format.internalFormat, m_Width, m_Height, 0, specs.format.format, (GLenum)specs.pixelType, 0) : 
			glTexImage2D(GL_TEXTURE_2D, 0, specs.format.internalFormat, m_Width, m_Height, 0, specs.format.format, (GLenum)specs.pixelType, data);

		if (specs.generateMipmap) glGenerateMipmap(GL_TEXTURE_2D);

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
