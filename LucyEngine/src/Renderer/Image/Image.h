#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	typedef int32_t TextureSlot;
	typedef uint32_t TextureFormat;

	enum class ImageType {
		Type2D, Type3D
	};

	enum class PixelType {
		Byte = 0x1400,
		UnsignedByte = 0x1401,
		UnsignedShort565 = 0x8363,
		Short = 0x1402,
		Int = 0x1404,
		Float = 0x1406
	};

	struct ImageParameter {
		uint32_t U = 0, V = 0, W = 0;
		uint32_t Min = 0, Mag = 0;
	};

	//Helper struct for materials
	struct TextureType {
		uint32_t Type;
		std::string Name;
		uint32_t Slot;
		uint32_t Index;
	};

	struct ImageSpecification {
		const char* Path = nullptr;

		int32_t Width = 0, Height = 0; //gets replaced if path is available
		TextureFormat Format;
		ImageParameter Parameter;

		uint32_t Samples = 0;
		ImageType ImageType;
		bool GenerateMipmap = false;

		RefLucy<void> InternalInfo = nullptr;
	};

	struct VulkanRHIImageDesc {
		bool ImGuiUsage = false;
		bool GenerateSampler = false;
	};

	struct OpenGLRHIImageDesc {
		Lucy::PixelType PixelType = Lucy::PixelType::UnsignedByte;
		uint32_t AttachmentIndex;
		TextureSlot Slot = -1;
		TextureFormat InternalFormat;
		bool DisableReadWriteBuffer = false;
	};

	//Vulkan: Descriptor Set
	//OpenGL: ID
	typedef void* ImageID;

	class Image2D
	{
	public:
		static RefLucy<Image2D> Create(ImageSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;

		ImageID GetID() const { return m_ID; }
	protected:
		Image2D(ImageSpecification& specs);

		ImageSpecification m_Specs;
		int32_t m_Channels = 0;
		int32_t m_Width;
		int32_t m_Height;
		ImageID m_ID = 0;
	};
}

