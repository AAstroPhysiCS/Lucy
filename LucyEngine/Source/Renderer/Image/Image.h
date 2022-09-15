#pragma once

#include "glm/gtc/integer.hpp"

namespace Lucy {

	enum class ImageType {
		Type2DDepth,
		Type2DColor,
		Type2DArrayColor,

		Type3DColor,

		TypeCubeColor
	};

	enum class ImageAddressMode {
		REPEAT, 
		CLAMP_TO_EDGE, 
		CLAMP_TO_BORDER
	};

	enum class ImageFilterMode {
		NEAREST, 
		LINEAR
	};

	enum class ImageFormat {
		R8G8B8A8_UNORM,
		R8G8B8A8_UINT,
		R8G8B8A8_SRGB,

		B8G8R8A8_UNORM,
		B8G8R8A8_UINT,
		B8G8R8A8_SRGB,

		D32_SFLOAT,

		R16G16B16A16_SFLOAT,
		R16G16B16A16_UINT,
		R16G16B16A16_UNORM,

		R32G32B32A32_SFLOAT,
		R32G32B32A32_UINT,

		R32G32B32_SFLOAT,

		R32_SFLOAT,
		R32_UINT,

		Unknown
	};

	//to be implemented by CLIENT
	uint32_t GetAPIImageFormat(ImageFormat format);
	ImageFormat GetLucyImageFormat(uint32_t format);

	struct ImageParameter {
		ImageAddressMode U = ImageAddressMode::REPEAT;
		ImageAddressMode V = ImageAddressMode::REPEAT;
		ImageAddressMode W = ImageAddressMode::REPEAT;
		ImageFilterMode Min = ImageFilterMode::LINEAR;
		ImageFilterMode Mag = ImageFilterMode::LINEAR;
	};

	using ImageFlags = uint32_t;

	//Helper struct for materials
	struct MaterialImageType {
		uint32_t Type;
		std::string Name;
		uint32_t Index;
	};

	struct ImageCreateInfo {
		int32_t Width = 0, Height = 0; //gets replaced if path is available
		ImageType ImageType;
		ImageFormat Format;
		ImageParameter Parameter;
		ImageFlags Flags = 0;

		uint32_t Samples = 1;
		uint32_t Layers = 1;
		
		bool ImGuiUsage = false;
		bool GenerateSampler = false;
		bool GenerateMipmap = false;
	};

	//Vulkan: Descriptor Set
	using ImageImGuiID = void*;

	class VulkanImage2D; //for copy constructor

	class Image {
	public:
		virtual ~Image() = default;

		static Ref<Image> Create(const std::string& path, ImageCreateInfo& createInfo);
		static Ref<Image> Create(const Ref<VulkanImage2D>& other);
		static Ref<Image> Create(ImageCreateInfo& createInfo);

		static Ref<Image> CreateCube(const std::string& path, ImageCreateInfo& createInfo);
		static Ref<Image> CreateCube(ImageCreateInfo& createInfo);

		virtual void Destroy() = 0;

		const std::string& GetPath() const { return m_Path; }
		const int32_t& GetChannels() const { return m_Channels; }
		const int32_t& GetWidth() const { return m_Width; }
		const int32_t& GetHeight() const { return m_Height; }

		ImageImGuiID GetImGuiID() const { return m_ImGuiID; }
	protected:
		//Loads an asset
		Image(const std::string& path, ImageCreateInfo& createInfo);
		//Creates an empty image
		Image(ImageCreateInfo& createInfo);

		ImageCreateInfo m_CreateInfo;
		int32_t m_Channels = 0;
		int32_t m_Width = 0;
		int32_t m_Height = 0;

		ImageImGuiID m_ImGuiID = 0;

		uint32_t m_MaxMipLevel = 1;

		std::string m_Path;
	};
}