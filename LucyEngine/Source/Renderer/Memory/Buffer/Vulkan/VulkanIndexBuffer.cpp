#include "lypch.h"
#include "VulkanIndexBuffer.h"

#include "Renderer/Renderer.h"
#include <Renderer/Device/VulkanRenderDevice.h>

namespace Lucy {

	VulkanIndexBuffer::VulkanIndexBuffer(size_t size, const Ref<VulkanRenderDevice>& device)
		: IndexBuffer(size), m_VulkanDevice(device) {
		RTCreate(size); //staging buffer allocation
	}

	void VulkanIndexBuffer::RTCreate(size_t size) {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanIndexBuffer::RTBind(const VulkanIndexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
		vkCmdBindIndexBuffer(info.CommandBuffer, m_BufferHandle, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::RTLoadToDevice() {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();

		void* data;
		allocator.MapMemory(m_StagingBufferVma, data);
		memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
		allocator.UnmapMemory(m_StagingBufferVma);

		allocator.CreateVulkanBufferVma(VulkanBufferUsage::GPUOnly, m_Data.size() * sizeof(float),
										VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_BufferHandle, m_BufferVma);
		Renderer::DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

		allocator.DestroyBuffer(m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanIndexBuffer::RTDestroyResource() {
		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.DestroyBuffer(m_BufferHandle, m_BufferVma);
	}
}