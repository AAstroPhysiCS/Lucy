#include "lypch.h"
#include "VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanRenderDevice.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Memory/VulkanAllocator.h"

namespace Lucy {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size)
		: VertexBuffer(size) {
		Renderer::Enqueue([&, size]() {
			Create(size);
		});
	}

	void VulkanVertexBuffer::Create(uint32_t size) {
		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanVertexBuffer::Bind(const VertexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
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

			allocator.CreateVulkanBufferVma(VulkanBufferUsage::GPUOnly, m_Data.size() * sizeof(float),
											VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_BufferHandle, m_BufferVma);
			Renderer::GetCurrentRenderDevice().As<VulkanRenderDevice>()->DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			vmaDestroyBuffer(allocator.GetVmaInstance(), m_StagingBufferHandle, m_StagingBufferVma);
		});
	}

	void VulkanVertexBuffer::DestroyHandle() {
		VulkanAllocator& allocator = VulkanAllocator::Get();
		vmaDestroyBuffer(allocator.GetVmaInstance(), m_BufferHandle, m_BufferVma);
	}
}