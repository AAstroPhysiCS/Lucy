#include "lypch.h"
#include "VulkanIndexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Memory/VulkanAllocator.h"

namespace Lucy {

	VulkanIndexBuffer::VulkanIndexBuffer(size_t size)
		: IndexBuffer(size) {
		Renderer::EnqueueToRenderThread([&, size]() {
			Create(size); //staging buffer allocation
		});
	}

	void VulkanIndexBuffer::Create(size_t size) {
		VulkanAllocator& allocator = VulkanAllocator::Get(); 
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanIndexBuffer::Bind(const VulkanIndexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
		vkCmdBindIndexBuffer(info.CommandBuffer, m_BufferHandle, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::LoadToGPU() {
		Renderer::EnqueueToRenderThread([&]() {
			VulkanAllocator& allocator = VulkanAllocator::Get();

			void* data;
			allocator.MapMemory(m_StagingBufferVma, data);
			memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
			allocator.UnmapMemory(m_StagingBufferVma);

			allocator.CreateVulkanBufferVma(VulkanBufferUsage::GPUOnly, m_Data.size() * sizeof(float),
											VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_BufferHandle, m_BufferVma);
			Renderer::DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			allocator.DestroyBuffer(m_StagingBufferHandle, m_StagingBufferVma);
		});
	}

	void VulkanIndexBuffer::DestroyHandle() {
		VulkanAllocator::Get().DestroyBuffer(m_BufferHandle, m_BufferVma);
	}
}