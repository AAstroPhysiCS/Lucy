#include "lypch.h"
#include "VulkanUniformBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/VulkanAllocator.h"

#include "glm/glm.hpp"

namespace Lucy {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding, VulkanDescriptorSet& descriptorSet)
		: m_DescriptorSet(descriptorSet), m_Binding(binding) {
		constexpr uint32_t maxFramesInFlight = VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferVma.resize(maxFramesInFlight, VK_NULL_HANDLE);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		
		for (uint32_t i = 0; i < maxFramesInFlight; i++) {
			allocator.CreateVulkanBufferVMA(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_Buffers[i], m_BufferVma[i]);
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
		VulkanAllocator& allocator = VulkanAllocator::Get();

		void* dataLocal;
		vmaMapMemory(allocator.GetVMAInstance(), m_BufferVma[VulkanSwapChain::GetCurrentFrameIndex()], &dataLocal);
		memcpy(dataLocal, data, size);
		vmaUnmapMemory(allocator.GetVMAInstance(), m_BufferVma[VulkanSwapChain::GetCurrentFrameIndex()]);
	}

	void VulkanUniformBuffer::WriteToSets(uint32_t index) {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_Buffers[index];
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet setWrite{};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.dstSet = m_DescriptorSet.GetSetBasedOffCurrentFrame(index);
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
		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
			vmaDestroyBuffer(allocator.GetVMAInstance(), m_Buffers[i], m_BufferVma[i]);
	}
}