#include "lypch.h"
#include "VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Memory/VulkanAllocator.h"

namespace Lucy {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size)
		: VertexBuffer(size) {
		Renderer::EnqueueToRenderThread([&, size]() {
			Create(size); //staging buffer allocation
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
		Renderer::EnqueueToRenderThread([&]() {
			VulkanAllocator& allocator = VulkanAllocator::Get();

			void* data;
			allocator.MapMemory(m_StagingBufferVma, data);
			memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
			allocator.UnmapMemory(m_StagingBufferVma);

			allocator.CreateVulkanBufferVma(VulkanBufferUsage::GPUOnly, m_Data.size() * sizeof(float),
											VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_BufferHandle, m_BufferVma);
			Renderer::DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			allocator.DestroyBuffer(m_StagingBufferHandle, m_StagingBufferVma);
		});
	}

	void VulkanVertexBuffer::DestroyHandle() {
		VulkanAllocator::Get().DestroyBuffer(m_BufferHandle, m_BufferVma);
	}
}