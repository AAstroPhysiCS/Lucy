#include "lypch.h"
#include "VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanRenderer.h"
#include "Renderer/Context/VulkanDevice.h"

#include "VulkanBufferUtils.h"

namespace Lucy {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size)
		: VertexBuffer(size) {
		Renderer::Submit([this, size]() {
			Create(size);
		});
	}

	VulkanVertexBuffer::VulkanVertexBuffer()
		: VertexBuffer() {
	}

	void VulkanVertexBuffer::Create(uint32_t size) {
		VulkanBufferUtils::CreateVulkanBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					 m_StagingBufferHandle, m_StagingBufferMemory);
	}

	void VulkanVertexBuffer::Bind(const VertexBindInfo& info) {
		VkDeviceSize offset[] = { 0 };
		vkCmdBindVertexBuffers(info.CommandBuffer, 0, 1, &m_BufferHandle, offset);
	}

	void VulkanVertexBuffer::Unbind() {
		//Empty
	}

	void VulkanVertexBuffer::AddData(const std::vector<float>& dataToAdd) {
		m_Data.insert(m_Data.end(), dataToAdd.begin(), dataToAdd.end());
	}

	void VulkanVertexBuffer::Load() {
		Renderer::Submit([&]() {
			VkDevice device = VulkanDevice::Get().GetLogicalDevice();

			void* data;
			vkMapMemory(device, m_StagingBufferMemory, 0, m_Data.size() * sizeof(float), 0, &data);
			memcpy(data, m_Data.data(), m_Data.size() * sizeof(float));
			vkUnmapMemory(device, m_StagingBufferMemory);

			VulkanBufferUtils::CreateVulkanBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_BufferHandle, m_BufferMemory);
			As(Renderer::GetCurrentRenderer(), VulkanRenderer)->DirectCopyBuffer(m_StagingBufferHandle, m_BufferHandle, m_Data.size() * sizeof(float));

			vkDestroyBuffer(device, m_StagingBufferHandle, nullptr);
			vkFreeMemory(device, m_StagingBufferMemory, nullptr);
		});
	}

	void VulkanVertexBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDestroyBuffer(device, m_BufferHandle, nullptr);
		vkFreeMemory(device, m_BufferMemory, nullptr);
	}
}