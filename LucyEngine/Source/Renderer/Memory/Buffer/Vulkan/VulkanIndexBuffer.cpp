#include "lypch.h"
#include "VulkanIndexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Memory/VulkanAllocator.h"

namespace Lucy {

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: IndexBuffer(size) {
		Renderer::EnqueueToRenderThread([&, size]() {
			Create(size); //staging buffer allocation
		});
	}

	void VulkanIndexBuffer::Create(uint32_t size) {
		VulkanAllocator& allocator = VulkanAllocator::Get(); 
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanIndexBuffer::Bind(const IndexBindInfo& info) {
		LUCY_ASSERT(m_BufferHandle);
		vkCmdBindIndexBuffer(info.CommandBuffer, m_BufferHandle, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::Unbind() {
		//Empty
	}

	void VulkanIndexBuffer::LoadToGPU() {
		Renderer::EnqueueToRenderThread([&]() {
			VulkanAllocator& allocator = VulkanAllocator::Get();
			VmaAllocator vmaAllocatorHandle = allocator.GetVmaInstance();

			void* data;
			vmaMapMemory(vmaAllocatorHandle, m_StagingBufferVma, &data);
			memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
			vmaUnmapMemory(vmaAllocatorHandle, m_StagingBufferVma);

			allocator.CreateVulkanBufferVma(VulkanBufferUsage::GPUOnly, m_Data.size() * sizeof(float),
											VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_BufferHandle, m_BufferVma);
			Renderer::DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			vmaDestroyBuffer(vmaAllocatorHandle, m_StagingBufferHandle, m_StagingBufferVma);
		});
	}

	void VulkanIndexBuffer::DestroyHandle() {
		VmaAllocator vmaAllocator = VulkanAllocator::Get().GetVmaInstance();
		vmaDestroyBuffer(vmaAllocator, m_BufferHandle, m_BufferVma);
	}
}