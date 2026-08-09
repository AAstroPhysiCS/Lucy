#include "lypch.h"
#include "VulkanIndexBuffer.h"

#include "Renderer/Renderer.h"
#include <Renderer/Device/VulkanRenderDevice.h>

namespace Lucy {

	VulkanIndexBuffer::VulkanIndexBuffer(size_t size, const Ref<VulkanRenderDevice>& device)
		: IndexBuffer(size) {
		RTCreate(device, size); //staging buffer allocation
	}

	void VulkanIndexBuffer::RTCreate(const Ref<VulkanRenderDevice>& device, size_t size) {
		VulkanAllocator& allocator = device->GetAllocator();
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, size * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false,
										m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanIndexBuffer::RTBind(const VulkanIndexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
		vkCmdBindIndexBuffer(info.CommandBuffer, m_BufferHandle, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::RTLoadToDevice(RenderDevice* device) {
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
		VulkanAllocator& allocator = vulkanDevice->GetAllocator();

		void* data;
		allocator.MapMemory(m_StagingBufferVma, data);
		memcpy(data, m_Data.data(), m_Data.size() * sizeof(uint32_t));
		allocator.UnmapMemory(m_StagingBufferVma);

		allocator.CreateVulkanBufferVma(VulkanBufferUsage::GPUOnly, m_Data.size() * sizeof(uint32_t),
										VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, false, m_BufferHandle, m_BufferVma);
		Renderer::RTDirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(uint32_t));

		allocator.DestroyBuffer(m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanIndexBuffer::RTDestroyResource(RenderDevice* device) {
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.DestroyBuffer(m_BufferHandle, m_BufferVma);
	}
}