#pragma once

#include "../UniformBuffer.h"
#include "Renderer/Image/VulkanImage.h"

namespace Lucy {

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(UniformBufferCreateInfo& createInfo);
		virtual ~VulkanUniformBuffer() = default;

		void LoadToGPU() override;
		void DestroyHandle() override;

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
		
		void LoadToGPU() override;

		uint32_t BindImage(Ref<VulkanImage2D> image);
		void Clear();
		void DestroyHandle() override;
	private:
		std::vector<VkDescriptorImageInfo> m_ImageInfos;
	};
}