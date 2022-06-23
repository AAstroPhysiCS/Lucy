#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	typedef uint32_t TextureFormat;
	typedef int32_t TextureSlot;

	enum class ImageType {
		Type2D, Type3D
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
		int32_t Width = 0, Height = 0; //gets replaced if path is available
		TextureFormat Format;
		ImageParameter Parameter;

		uint32_t Samples = 1;
		ImageType ImageType;
		bool GenerateMipmap = false;

		Ref<void> InternalInfo = nullptr;
	};

	struct VulkanRHIImageDesc {
		bool ImGuiUsage = false;
		bool GenerateSampler = false;
	private:
		bool DepthEnable = false; //for framebuffer to use for its depth image

		friend class VulkanFrameBuffer;
		friend class VulkanImage2D;
	};

	struct OpenGLRHIImageDesc {

		enum class PixelType {
			Byte = 0x1400,
			UnsignedByte = 0x1401,
			UnsignedShort565 = 0x8363,
			Short = 0x1402,
			Int = 0x1404,
			Float = 0x1406
		};
		
		PixelType PixelType = PixelType::UnsignedByte;
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
		virtual ~Image2D() = default;

		static Ref<Image2D> Create(const std::string& path, ImageSpecification& specs);
		static Ref<Image2D> Create(ImageSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;

		ImageID GetID() const { return m_ID; }
	protected:
		//Loads an asset
		Image2D(const std::string& path, ImageSpecification& specs);
		//Creates an empty image
		Image2D(ImageSpecification& specs);

		ImageSpecification m_Specs;
		int32_t m_Channels = 0;
		int32_t m_Width = 0;
		int32_t m_Height = 0;
		ImageID m_ID = 0;

		const std::string m_Path;
	};
}