#pragma once

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

	enum class ImageType : uint8_t {
		Type2D,
		Type3D,
		TypeCube
	};

	enum class ImageAddressMode : uint8_t {
		REPEAT, 
		CLAMP_TO_EDGE, 
		CLAMP_TO_BORDER
	};

	enum class ImageFilterMode : uint8_t {
		NEAREST, 
		LINEAR
	};

	enum class ImageFormat {
		Unknown = -1,

		R8G8B8A8_UNORM,
		R8G8B8A8_UINT,
		R8G8B8A8_SRGB,

		B8G8R8A8_UNORM,
		B8G8R8A8_UINT,
		B8G8R8A8_SRGB,

		D32_SFLOAT,

		R16G16_SFLOAT,

		R16G16B16A16_SFLOAT,
		R16G16B16A16_UINT,
		R16G16B16A16_UNORM,

		R32G32B32A32_SFLOAT,
		R32G32B32A32_UINT,

		R32G32B32_SFLOAT,

		R32G32_SFLOAT,

		R32_SFLOAT,
		R32_UINT
	};

	enum class ImageUsage : uint8_t {
		Unknown = 1 << 1,
		AsColorAttachment = 1 << 2,
		AsColorTransferAttachment = 1 << 3,
		AsColorStorageTransferAttachment = 1 << 4,
		AsDepthAttachment = 1 << 5,
		AsTransientColorAttachment = 1 << 6,
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

	struct ImageCreateInfo {
		uint32_t Width = 0, Height = 0; //gets replaced if path is available
		ImageType ImageType;
		ImageUsage ImageUsage = ImageUsage::Unknown;
		ImageFormat Format;
		ImageParameter Parameter;

		uint32_t Samples = 1;
		uint32_t Layers = 1;
		
		bool GenerateSampler = false;
		bool GenerateMipmap = false;
		bool ImGuiUsage = false;
	};

	//Vulkan: Descriptor Set
	using ImageImGuiID = void*;

	class Image : public RenderResource {
	public:
		virtual ~Image() = default;

		inline const std::filesystem::path& GetPath() const { return m_Path; }
		inline int32_t GetChannels() const { return m_Channels; }
		inline int32_t GetWidth() const { return m_CreateInfo.Width; }
		inline int32_t GetHeight() const { return m_CreateInfo.Height; }

		inline ImageFormat GetFormat() const { return m_CreateInfo.Format; }
		inline uint32_t GetSamples() const { return m_CreateInfo.Samples; }

		inline uint32_t GetLayerCount() const { return m_CreateInfo.Layers; }
		inline uint32_t GetMaxMipLevel() const { return m_MaxMipLevel; }

		ImageImGuiID GetImGuiID() const { return m_ImGuiID; }
	protected:
		//Creates an empty image
		Image(const ImageCreateInfo& createInfo)
			: RenderResource("Image"), m_CreateInfo(createInfo) {
			if (m_CreateInfo.GenerateMipmap)
				m_MaxMipLevel = (uint32_t)glm::floor(glm::log2(glm::max(m_CreateInfo.Width, m_CreateInfo.Height))) + 1u;
			LUCY_ASSERT(m_CreateInfo.ImageUsage != ImageUsage::Unknown, "Image usage is unknown.");
		}

		//Loads an asset
		Image(const std::filesystem::path& path, const ImageCreateInfo& createInfo)
			: RenderResource("Image"), m_CreateInfo(createInfo), m_Path(path) {
		}

		ImageCreateInfo m_CreateInfo;
		int32_t m_Channels = 0;
		uint32_t m_MaxMipLevel = 1;

		ImageImGuiID m_ImGuiID = 0;

		std::filesystem::path m_Path;
	};
}