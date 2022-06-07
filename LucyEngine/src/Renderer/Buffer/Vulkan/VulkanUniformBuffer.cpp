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
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		const uint32_t maxFramesInFlight = swapChain.GetMaxFramesInFlight();
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferVma.resize(maxFramesInFlight, VK_NULL_HANDLE);
	}

	void VulkanUniformBuffer::Bind() {
		//no use
	}

	void VulkanUniformBuffer::Unbind() {
		//no use
	}

	void VulkanUniformBuffer::SetData(void* data, uint32_t size, uint32_t offset) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		VulkanAllocator& allocator = VulkanAllocator::Get();

		if (!m_Allocated) {
			for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++) {
				allocator.CreateVulkanBufferVma(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_Buffers[i], m_BufferVma[i]);
			}
			m_Allocated = true;
		}

		void* dataLocal;
		vmaMapMemory(allocator.GetVmaInstance(), m_BufferVma[swapChain.GetCurrentFrameIndex()], &dataLocal);
		memcpy(dataLocal, data, size);
		vmaUnmapMemory(allocator.GetVmaInstance(), m_BufferVma[swapChain.GetCurrentFrameIndex()]);
	}

	void VulkanUniformBuffer::Update(RefLucy<VulkanImage2D> image) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		
		const uint32_t index = swapChain.GetCurrentFrameIndex();
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		
		VkWriteDescriptorSet setWrite{};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.dstSet = m_DescriptorSet.GetSetBasedOffCurrentFrame(index);
		setWrite.dstBinding = m_Binding;
		setWrite.dstArrayElement = 0;
		setWrite.descriptorCount = 1;
		setWrite.pTexelBufferView = nullptr;

		if (image) { //is a uniform buffer suitable for a sampler
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = image->GetCurrentLayout();
			imageInfo.imageView = image->GetImageView().GetVulkanHandle();
			imageInfo.sampler = image->GetImageView().GetSampler();

			setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			setWrite.pImageInfo = &imageInfo;
			
			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
		} else {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_Buffers[index];
			bufferInfo.offset = 0;
			bufferInfo.range = VK_WHOLE_SIZE;

			setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			setWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr); //reason for duplicate code is that bufferInfo and imageInfo is stack allocated
		}
	}

	void VulkanUniformBuffer::Destroy() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
			vmaDestroyBuffer(allocator.GetVmaInstance(), m_Buffers[i], m_BufferVma[i]);
		m_Allocated = false;
	}
}