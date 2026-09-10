#include "lypch.h"
#include "VulkanIndexBuffer.h"

#include "Renderer/Renderer.h"
#include <Renderer/Device/VulkanRenderDevice.h>

namespace Lucy {

	VulkanIndexBuffer::VulkanIndexBuffer(size_t size, const Ref<VulkanRenderDevice>& device)
		: IndexBuffer(size) {
		RTCreate(device, size);
	}

	void VulkanIndexBuffer::RTCreate(const Ref<VulkanRenderDevice>& device, size_t size) {
		VulkanAllocator& allocator = device->GetAllocator();
		allocator.CreateVulkanBufferVma(MemoryUsage::GPUOnly, size * sizeof(uint32_t),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, false, m_BufferHandle, m_BufferVma);
	}

	void VulkanIndexBuffer::RTBind(const VulkanIndexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
		vkCmdBindIndexBuffer(info.CommandBuffer, m_BufferHandle, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::RTLoadToDevice(RenderDevice* device) {
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
		vulkanDevice->GetUploadManager()->EnqueueUploadBuffer(m_BufferHandle, 0, m_Data.data(), m_Data.size() * sizeof(uint32_t));
	}

	void VulkanIndexBuffer::RTDestroyResource(RenderDevice* device) {
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.DestroyBuffer(m_BufferHandle, m_BufferVma);
	}
}