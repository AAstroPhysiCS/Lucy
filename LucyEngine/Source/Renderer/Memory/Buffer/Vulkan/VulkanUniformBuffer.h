#pragma once

#include "../UniformBuffer.h"
#include "Renderer/Image/VulkanImage2D.h"

namespace Lucy {

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(UniformBufferCreateInfo& createInfo);
		virtual ~VulkanUniformBuffer() = default;

		void LoadToGPU() final override;
		void DestroyHandle() final override;

		inline VkBuffer GetVulkanBufferHandle(const uint32_t index) { return m_Buffers[index]; }
	private:
		std::vector<VkBuffer> m_Buffers;
		std::vector<VmaAllocation> m_BufferVma;
	};

	class VulkanUniformImageBuffer : public UniformBuffer {
	public:
		VulkanUniformImageBuffer(UniformBufferCreateInfo& createInfo);
		virtual ~VulkanUniformImageBuffer() = default;

		inline const std::vector<VkDescriptorImageInfo>& GetImageInfos() const { return m_ImageInfos; }
		
		void LoadToGPU() final override;

		uint32_t BindImage(Ref<VulkanImage2D> image);
		uint32_t BindImage(VkImageView imageView, VkImageLayout layout, VkSampler sampler);
		void Clear();
		void DestroyHandle() final override;
	private:
		std::vector<VkDescriptorImageInfo> m_ImageInfos;
	};
}