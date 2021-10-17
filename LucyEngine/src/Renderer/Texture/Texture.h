#pragma once

#define STB_IMAGE_IMPLEMENTATION

#include "../../Core/Base.h"

namespace Lucy {

	enum class PixelType {
		Float = 0x1406, 
		UnsignedByte = 0x1401
	};

	struct TextureFormat {
		uint32_t internalFormat;
		uint32_t format;
	};

	struct TextureParameter {
		uint32_t R = 0, S = 0, T = 0;
		uint32_t min = 0, mag = 0;
	};

	struct TextureSpecification {
		const char* path = nullptr;
		Lucy::PixelType pixelType = Lucy::PixelType::UnsignedByte;
		TextureFormat format;
		TextureParameter parameter;
		int32_t width = 0, height = 0; //gets replaced if path is available
		bool generateMipmap;
		uint32_t attachmentIndex;
	};

	class Texture2D
	{
	public:
		static RefLucy<Texture2D> Create(TextureSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;

		inline uint32_t GetID() const { return m_Id; }

	protected:
		Texture2D(TextureSpecification& specs);

		TextureSpecification m_Specs;
		uint32_t m_Id = 0;
		int32_t m_Channels = 0;
		int32_t m_Width;
		int32_t m_Height;
	};
}

