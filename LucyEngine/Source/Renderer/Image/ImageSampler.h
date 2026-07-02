#pragma once

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

	class ImageSampler {
	public:
		ImageSampler(const ImageSamplerCreateInfo& createInfo)
			: m_CreateInfo(createInfo) {
		}
		virtual ~ImageSampler() = default;

		virtual void RTDestroyResource() = 0;

		inline ImageParameter GetParameter() const { return m_CreateInfo.Parameter; }

		inline float GetMipmapLevel() const { return m_CreateInfo.MipmapLevel; }
		inline bool IsMipmapEnabled() const { return m_CreateInfo.MipmapEnabled; }
	protected:
		ImageSampler() = default;

		const ImageSamplerCreateInfo& GetCreateInfo() const { return m_CreateInfo; }
	private:
		ImageSamplerCreateInfo m_CreateInfo;
	};
}