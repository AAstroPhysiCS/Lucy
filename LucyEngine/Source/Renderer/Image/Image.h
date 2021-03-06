#pragma once

#include "Core/Base.h"

namespace Lucy {

	typedef uint32_t ImageFormat;

	enum class ImageType {
		Type2D, Type3D
	};

	enum class ImageTarget {
		Color, Depth
	};

	struct ImageParameter {
		uint32_t U = 0, V = 0, W = 0;
		uint32_t Min = 0, Mag = 0;
	};

	//Helper struct for materials
	struct MaterialImageType {
		uint32_t Type;
		std::string Name;
		uint32_t Index;
	};

	struct ImageCreateInfo {
		int32_t Width = 0, Height = 0; //gets replaced if path is available
		ImageFormat Format;
		ImageTarget Target;
		ImageParameter Parameter;

		uint32_t Samples = 1;
		ImageType ImageType;
		bool GenerateMipmap = false;

		Ref<void> InternalInfo = nullptr;
	};

	struct VulkanRHIImageDesc {
		bool ImGuiUsage = false;
		bool GenerateSampler = false;
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
		ImageFormat InternalFormat;
		bool DisableReadWriteBuffer = false;
	};

	//Vulkan: Descriptor Set
	//OpenGL: ID
	typedef void* ImageImGuiID;

	class Image2D
	{
	public:
		virtual ~Image2D() = default;

		static Ref<Image2D> Create(const std::string& path, ImageCreateInfo& createInfo);
		static Ref<Image2D> Create(ImageCreateInfo& createInfo);

		virtual void Destroy() = 0;

		const int32_t& GetChannels() const { return m_Channels; }
		const int32_t& GetWidth() const { return m_Width; }
		const int32_t& GetHeight() const { return m_Height; }

		ImageImGuiID GetImGuiID() const { return m_ImGuiID; }
	protected:
		//Loads an asset
		Image2D(const std::string& path, ImageCreateInfo& createInfo);
		//Creates an empty image
		Image2D(ImageCreateInfo& createInfo);

		ImageCreateInfo m_CreateInfo;
		int32_t m_Channels = 0;
		int32_t m_Width = 0;
		int32_t m_Height = 0;

		ImageImGuiID m_ImGuiID = 0;

		const std::string m_Path;
	};
}