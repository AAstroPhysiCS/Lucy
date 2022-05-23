#include "lypch.h"
#include "VulkanIndexBuffer.h"

#include "vulkan/vulkan.h"
#include "Renderer/Renderer.h"

#include "Renderer/VulkanAllocator.h"
#include "Renderer/VulkanRHI.h"

namespace Lucy {

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: IndexBuffer(size) {
		Renderer::Enqueue([this, size]() {
			Create(size);
		});
	}

	VulkanIndexBuffer::VulkanIndexBuffer()
		: IndexBuffer() {
		Renderer::Enqueue([this]() {
			Create();
		});
	}

	void VulkanIndexBuffer::Create(uint32_t size) {
		VulkanAllocator& allocator = VulkanAllocator::Get(); 
		allocator.CreateVulkanBufferVma(size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_StagingBufferHandle, m_StagingBufferVma);
	}

	void VulkanIndexBuffer::Bind(const IndexBindInfo& info) {
		vkCmdBindIndexBuffer(info.CommandBuffer, m_BufferHandle, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::Unbind() {
		//Empty
	}

	void VulkanIndexBuffer::AddData(const std::vector<uint32_t>& dataToAdd) {
		m_Data.insert(m_Data.end(), dataToAdd.begin(), dataToAdd.end());
	}

	void VulkanIndexBuffer::Load() {
		Renderer::Enqueue([&]() {
			VulkanAllocator& allocator = VulkanAllocator::Get();
			VmaAllocator vmaAllocatorHandle = allocator.GetVmaInstance();

			void* data;
			vmaMapMemory(vmaAllocatorHandle, m_StagingBufferVma, &data);
			memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
			vmaUnmapMemory(vmaAllocatorHandle, m_StagingBufferVma);

			allocator.CreateVulkanBufferVma(m_Size * sizeof(float), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_BufferHandle, m_BufferVma);
			As(Renderer::GetCurrentRenderer(), VulkanRHI)->DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			vmaDestroyBuffer(vmaAllocatorHandle, m_StagingBufferHandle, m_StagingBufferVma);
		});
	}

	void VulkanIndexBuffer::Destroy() {
		VmaAllocator vmaAllocator = VulkanAllocator::Get().GetVmaInstance();
		vmaDestroyBuffer(vmaAllocator, m_BufferHandle, m_BufferVma);
	}
}