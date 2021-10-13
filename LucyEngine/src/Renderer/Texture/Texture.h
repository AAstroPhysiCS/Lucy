#pragma once

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
		uint32_t R, S, T;
		uint32_t min, mag;
	};

	struct TextureSpecification {
		const char* path;
		Lucy::PixelType pixelType = Lucy::PixelType::UnsignedByte;
		TextureFormat format;
		TextureParameter parameter;
		int32_t width, height; //gets replaced if path is available
		bool generateMipmap;
	};

	class Texture2D
	{
	public:
		static RefLucy<Texture2D> Create(TextureSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;

		uint32_t GetID();

	protected:
		Texture2D(TextureSpecification& specs);

		TextureSpecification m_Specs;
		uint32_t m_Id;
		int32_t m_Width;
		int32_t m_Height;
		int32_t m_Channels;
	};
}

