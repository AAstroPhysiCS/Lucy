#pragma once

#define STB_IMAGE_IMPLEMENTATION

#include "../../Core/Base.h"

namespace Lucy {

	enum class PixelType {
		Byte = 0x1400,
		UnsignedByte = 0x1401,
		UnsignedShort565 = 0x8363,
		Short = 0x1402,
		Int = 0x1404,
		Float = 0x1406
	};

	struct TextureFormat {
		uint32_t InternalFormat;
		uint32_t Format;
	};

	struct TextureParameter {
		uint32_t R = 0, S = 0, T = 0;
		uint32_t Min = 0, Mag = 0;
	};

	//Helper struct for materials
	struct TextureType {
		uint32_t Type;
		std::string Name;
		uint32_t Slot;
		uint32_t Index;
	};

	typedef uint32_t TextureSlot;

	struct TextureSpecification {
		const char* Path = nullptr;
		Lucy::PixelType PixelType = Lucy::PixelType::UnsignedByte;
		
		TextureFormat Format;
		TextureParameter Parameter;
		
		int32_t Width = 0, Height = 0; //gets replaced if path is available
		
		bool DisableReadWriteBuffer = false;
		bool GenerateMipmap = false;

		uint32_t AttachmentIndex;
		int32_t Slot = -1;

		uint32_t Samples = 0;
	};

	class Texture2D
	{
	public:
		static RefLucy<Texture2D> Create(TextureSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;

		inline uint32_t GetID() const { return m_Id; }
		inline int32_t GetSlot() const { return m_Specs.Slot; }

	protected:
		Texture2D(TextureSpecification& specs);

		TextureSpecification m_Specs;
		uint32_t m_Id = 0;
		int32_t m_Channels = 0;
		int32_t m_Width;
		int32_t m_Height;
	};
}

