#pragma once

#include "../IndexBuffer.h"

namespace Lucy {

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer();
		VulkanIndexBuffer(uint32_t size);
		virtual ~VulkanIndexBuffer() = default;
		
		void Bind(const IndexBindInfo& info) override;
		void Unbind() override;
		void AddData(const std::vector<uint32_t>& dataToAdd) override;
		void Load() override;
		void Destroy() override;
	private:
		void Create(uint32_t size = 0);

		VkBuffer m_BufferHandle = VK_NULL_HANDLE;
		VkBuffer m_StagingBufferHandle = VK_NULL_HANDLE;
		VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory m_StagingBufferMemory = VK_NULL_HANDLE;
	};
}

