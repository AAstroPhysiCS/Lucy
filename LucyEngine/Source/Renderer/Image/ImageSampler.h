#pragma once

#include "Renderer/Device/RenderDeviceResource.h"

#include "Renderer/Image/Image.h"

namespace Lucy {

	struct ImageParameter {
		ImageAddressMode U = ImageAddressMode::REPEAT;
		ImageAddressMode V = ImageAddressMode::REPEAT;
		ImageAddressMode W = ImageAddressMode::REPEAT;
		ImageFilterMode Min = ImageFilterMode::LINEAR;
		ImageFilterMode Mag = ImageFilterMode::LINEAR;
	};

	struct ImageSamplerCreateInfo {
		bool MipmapEnabled = false;
		float MipmapLevel = 0.0f;
		ImageParameter Parameter;
	};

	class ImageSampler : public RenderDeviceResource {
	public:
		virtual ~ImageSampler() = default;

		ImageSampler(const ImageSampler&) = delete;
		ImageSampler& operator=(const ImageSampler&) = delete;
		ImageSampler(ImageSampler&&) = delete;
		ImageSampler& operator=(ImageSampler&&) = delete;

		inline ImageParameter GetParameter() const { return m_CreateInfo.Parameter; }

		inline float GetMipmapLevel() const { return m_CreateInfo.MipmapLevel; }
		inline bool IsMipmapEnabled() const { return m_CreateInfo.MipmapEnabled; }
	protected:
		ImageSampler(const ImageSamplerCreateInfo& createInfo)
			: RenderDeviceResource("ImageSampler"), m_CreateInfo(createInfo) {}
		ImageSampler() = default;

		const ImageSamplerCreateInfo& GetCreateInfo() const { return m_CreateInfo; }
	private:
		ImageSamplerCreateInfo m_CreateInfo;
	};
}