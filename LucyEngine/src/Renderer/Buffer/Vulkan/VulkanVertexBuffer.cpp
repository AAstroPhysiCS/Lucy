#include "lypch.h"
#include "VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanRHI.h"
#include "Renderer/Context/VulkanDevice.h"

#include "Renderer/VulkanAllocator.h"

namespace Lucy {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size)
		: VertexBuffer(size) {
		Renderer::Enqueue([&, size]() {
			Create(size);
		});
	}

	void VulkanVertexBuffer::Create(uint32_t size) {
		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanBufferVma(size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
										VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanVertexBuffer::Bind(const VertexBindInfo& info) {
		VkDeviceSize offset[] = { 0 };
		vkCmdBindVertexBuffers(info.CommandBuffer, 0, 1, &m_BufferHandle, offset);
	}

	void VulkanVertexBuffer::Unbind() {
		//Empty
	}

	void VulkanVertexBuffer::LoadToGPU() {
		Renderer::Enqueue([&]() {
			VulkanAllocator& allocator = VulkanAllocator::Get();

			void* data;
			vmaMapMemory(allocator.GetVmaInstance(), m_StagingBufferVma, &data);
			memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
			vmaUnmapMemory(allocator.GetVmaInstance(), m_StagingBufferVma);

			allocator.CreateVulkanBufferVma(m_Data.size() * sizeof(float), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
											VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_BufferHandle, m_BufferVma);
			As(Renderer::GetCurrentRenderer(), VulkanRHI)->DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			vmaDestroyBuffer(allocator.GetVmaInstance(), m_StagingBufferHandle, m_StagingBufferVma);
		});
	}

	void VulkanVertexBuffer::DestroyHandle() {
		VulkanAllocator& allocator = VulkanAllocator::Get();
		vmaDestroyBuffer(allocator.GetVmaInstance(), m_BufferHandle, m_BufferVma);
	}
}