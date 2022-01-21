#pragma once

#include "../VertexBuffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		void Bind(const VertexBindInfo& info);
		void Unbind();
		void AddData(const std::vector<float>& dataToAdd);
		void Load();
		void Destroy();

		VulkanVertexBuffer(uint32_t size);
		VulkanVertexBuffer();
		virtual ~VulkanVertexBuffer() = default;
	private:
		void Create(uint32_t size = 0);

		VkBuffer m_StagingBufferHandle = VK_NULL_HANDLE;
		VkBuffer m_BufferHandle = VK_NULL_HANDLE;
		VkDeviceMemory m_StagingBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
	};
}