#pragma once

namespace Lucy {

	enum class ImageType {
		Type2D, 
		Type3D
	};

	enum class ImageTarget {
		Color, 
		Depth
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

		B8G8R8A8_UNORM,
		B8G8R8A8_UINT,
		B8G8R8A8_SRGB,

		D32_SFLOAT,

		R16G16B16A16_SFLOAT,
		R16G16B16A16_UINT,
		R16G16B16A16_UNORM,

		R32G32B32A32_SFLOAT,
		R32G32B32A32_UINT,

		R32_SFLOAT,
		R32_UINT
	};

	//to be implemented by CLIENT
	extern uint32_t GetAPIImageFormat(ImageFormat format);
	extern ImageFormat GetLucyImageFormat(uint32_t format);

	struct ImageParameter {
		ImageAddressMode U = ImageAddressMode::REPEAT;
		ImageAddressMode V = ImageAddressMode::REPEAT;
		ImageAddressMode W = ImageAddressMode::REPEAT;
		ImageFilterMode Min = ImageFilterMode::LINEAR;
		ImageFilterMode Mag = ImageFilterMode::LINEAR;
	};

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
		ImageTarget Target;
		ImageParameter Parameter;

		//won't have an effect, if the image is being imported from path
		uint32_t AdditionalUsageFlags = 0;

		uint32_t Samples = 1;
		bool GenerateMipmap = false;

		bool ImGuiUsage = false;
		bool GenerateSampler = false;
	};

	//Vulkan: Descriptor Set
	typedef void* ImageImGuiID;

	class Image2D {
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