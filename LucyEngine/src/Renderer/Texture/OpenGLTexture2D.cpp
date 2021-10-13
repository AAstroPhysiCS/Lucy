#include "OpenGLTexture2D.h"

#include "../Renderer.h"

#include "stb/stb_image.h"

namespace Lucy {

	OpenGLTexture2D::OpenGLTexture2D(TextureSpecification& specs)
		: Texture2D(specs)
	{
		Renderer::Submit([&]() {
			//if you want to create a texture buffer, you can leave the path empty and the stbi_load would fail.
			uint8_t* data = stbi_load(specs.path, &m_Width, &m_Height, &m_Channels, 0);
			
			if (!data)
				LUCY_CRITICAL(std::string("Failed to load a texture. Texture name: ").append(specs.path));

			LUCY_ASSERT(m_Width == 0 || m_Height == 0);

			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);
			Bind();

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, specs.parameter.S);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, specs.parameter.T);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, specs.parameter.R);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, specs.parameter.min);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, specs.parameter.mag);

			glTexImage2D(GL_TEXTURE_2D, 0, specs.format.internalFormat, m_Width, m_Height, 0, specs.format.format, (GLenum)specs.pixelType, 0);
			if (specs.generateMipmap) glGenerateMipmap(GL_TEXTURE_2D);

			stbi_image_free(data);
			Unbind();
		});
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
