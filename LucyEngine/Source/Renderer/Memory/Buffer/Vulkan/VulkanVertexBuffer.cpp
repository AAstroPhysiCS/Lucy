#include "lypch.h"
#include "VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanVertexBuffer::VulkanVertexBuffer(size_t size, const Ref<VulkanRenderDevice>& device)
		: VertexBuffer(size) {
		RTCreate(device, size);
	}

	void VulkanVertexBuffer::RTCreate(const Ref<VulkanRenderDevice>& device, size_t size) {
		VulkanAllocator& allocator = device->GetAllocator();
		allocator.CreateVulkanBufferVma(MemoryUsage::GPUOnly, size * sizeof(float),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false, m_BufferHandle, m_BufferVma);
	}

	void VulkanVertexBuffer::RTBind(const VulkanVertexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
		VkDeviceSize offset[] = { 0 };
		vkCmdBindVertexBuffers(info.CommandBuffer, 0, 1, &m_BufferHandle, offset);
	}

	void VulkanVertexBuffer::RTLoadToDevice(const Ref<RenderDevice>& device) {
		auto vulkanDevice = device->As<VulkanRenderDevice>();
		vulkanDevice->GetUploadManager()->EnqueueUploadBuffer(m_BufferHandle, 0, m_Data.data(), m_Data.size() * sizeof(uint32_t));
	}

	void VulkanVertexBuffer::RTDestroyResource(RenderDevice* device) {
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.DestroyBuffer(m_BufferHandle, m_BufferVma);
	}
}