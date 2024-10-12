#include "lypch.h"
#include "VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanVertexBuffer::VulkanVertexBuffer(size_t size, const Ref<VulkanRenderDevice>& device)
		: VertexBuffer(size), m_VulkanDevice(device) {
		RTCreate(size); //staging buffer allocation
	}

	void VulkanVertexBuffer::RTCreate(size_t size) {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanVertexBuffer::RTBind(const VulkanVertexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
		VkDeviceSize offset[] = { 0 };
		vkCmdBindVertexBuffers(info.CommandBuffer, 0, 1, &m_BufferHandle, offset);
	}

	void VulkanVertexBuffer::RTLoadToDevice() {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();

		void* data;
		allocator.MapMemory(m_StagingBufferVma, data);
		memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
		allocator.UnmapMemory(m_StagingBufferVma);

		allocator.CreateVulkanBufferVma(VulkanBufferUsage::GPUOnly, m_Data.size() * sizeof(float),
										VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_BufferHandle, m_BufferVma);
		Renderer::DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

		allocator.DestroyBuffer(m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanVertexBuffer::RTDestroyResource() {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.DestroyBuffer(m_BufferHandle, m_BufferVma);
	}
}