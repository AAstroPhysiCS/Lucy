#pragma once

#include "../UniformBuffer.h"
#include "Renderer/Image/VulkanImage2D.h"

namespace Lucy {

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(const UniformBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanUniformBuffer() = default;

		void RTLoadToDevice() final override;

		inline VkBuffer GetVulkanBufferHandle(const uint32_t index) { return m_Buffers[index]; }
	private:
		void RTDestroyResource() final override;

		std::vector<VkBuffer> m_Buffers;
		std::vector<VmaAllocation> m_BufferVma;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}