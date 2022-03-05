#include "lypch.h"
#include "VulkanUniformBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanRenderer.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Buffer/Vulkan/VulkanBufferUtils.h"

#include "glm/glm.hpp"

namespace Lucy {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding, VulkanDescriptorSet& descriptorSet)
		: m_DescriptorSet(descriptorSet), m_Binding(binding) {
		constexpr uint32_t maxFramesInFlight = VulkanRenderer::MAX_FRAMES_IN_FLIGHT;
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferMemories.resize(maxFramesInFlight, VK_NULL_HANDLE);

		for (uint32_t i = 0; i < maxFramesInFlight; i++) {
			VulkanBufferUtils::CreateVulkanBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
												  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_Buffers[i], m_BufferMemories[i]);
		}
	}

	void VulkanUniformBuffer::Bind() {
		//no use
	}

	void VulkanUniformBuffer::Unbind() {
		//no use
	}

	void VulkanUniformBuffer::SetData(void* data, uint32_t size, uint32_t offset) {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		void* dataLocal;
		vkMapMemory(device, m_BufferMemories[VulkanRenderer::CURRENT_FRAME], offset, size, 0, &dataLocal);
		memcpy(dataLocal, data, size);
		vkUnmapMemory(device, m_BufferMemories[VulkanRenderer::CURRENT_FRAME]);
	}

	void VulkanUniformBuffer::WriteToSets(uint32_t index) {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_Buffers[index];
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet setWrite{};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.dstSet = m_DescriptorSet.GetSetBasedOffFrameIndex(index);
		setWrite.dstBinding = m_Binding;
		setWrite.dstArrayElement = 0;
		setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setWrite.descriptorCount = 1;
		setWrite.pBufferInfo = &bufferInfo;
		setWrite.pImageInfo = nullptr;
		setWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
	}

	void VulkanUniformBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		for (uint32_t i = 0; i < VulkanRenderer::MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device, m_Buffers[i], nullptr);
			vkFreeMemory(device, m_BufferMemories[i], nullptr);
		}
	}
}