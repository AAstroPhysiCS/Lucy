#include "lypch.h"
#include "VulkanIndexBuffer.h"

#include "vulkan/vulkan.h"
#include "Renderer/Renderer.h"

#include "VulkanBufferUtils.h"
#include "Renderer/VulkanRenderer.h"

namespace Lucy {

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: IndexBuffer(size) {
		Renderer::Submit([this, size]() {
			Create(size);
		});
	}

	VulkanIndexBuffer::VulkanIndexBuffer()
		: IndexBuffer() {
		Renderer::Submit([this]() {
			Create();
		});
	}

	void VulkanIndexBuffer::Create(uint32_t size) {
		VulkanBufferUtils::CreateVulkanBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
									VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
									m_StagingBufferHandle, m_StagingBufferMemory);
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
		Renderer::Submit([&]() {
			VkDevice device = VulkanDevice::Get().GetLogicalDevice();

			void* data;
			vkMapMemory(device, m_StagingBufferMemory, 0, m_Data.size() * sizeof(float), 0, &data);
			memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
			vkUnmapMemory(device, m_StagingBufferMemory);

			VulkanBufferUtils::CreateVulkanBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_BufferHandle, m_BufferMemory);
			As(Renderer::GetCurrentRenderer(), VulkanRenderer)->DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			vkDestroyBuffer(device, m_StagingBufferHandle, nullptr);
			vkFreeMemory(device, m_StagingBufferMemory, nullptr);
		});
	}

	void VulkanIndexBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkFreeMemory(device, m_BufferMemory, nullptr);
		vkDestroyBuffer(device, m_BufferHandle, nullptr);
	}
}