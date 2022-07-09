#include "lypch.h"
#include "VulkanUniformBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Memory/VulkanAllocator.h"

#include "glm/glm.hpp"

namespace Lucy {

	VulkanUniformBuffer::VulkanUniformBuffer(UniformBufferCreateInfo& createInfo)
		: UniformBuffer(createInfo), m_DescriptorSet(createInfo.InternalInfo.As<VulkanRHIUniformCreateInfo>()->DescriptorSet) {
		if (m_CreateInfo.BufferSize == 0) {
			LUCY_CRITICAL("Size is 0.");
			LUCY_ASSERT(false);
		}

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		
		const uint32_t maxFramesInFlight = swapChain.GetMaxFramesInFlight();
		m_Buffers.resize(maxFramesInFlight, VK_NULL_HANDLE);
		m_BufferVma.resize(maxFramesInFlight, VK_NULL_HANDLE);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
			allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, m_CreateInfo.BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_Buffers[i], m_BufferVma[i]);
	}

	void VulkanUniformBuffer::SetData(void* data, uint32_t size, uint32_t offset) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		VulkanAllocator& allocator = VulkanAllocator::Get();

		void* dataLocal;
		vmaMapMemory(allocator.GetVmaInstance(), m_BufferVma[swapChain.GetCurrentFrameIndex()], &dataLocal);
		memcpy(dataLocal, data, size);
		vmaUnmapMemory(allocator.GetVmaInstance(), m_BufferVma[swapChain.GetCurrentFrameIndex()]);
	}

	void VulkanUniformBuffer::Update() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		const uint32_t index = swapChain.GetCurrentFrameIndex();
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		VkWriteDescriptorSet setWrite{};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.dstSet = m_DescriptorSet->GetSetBasedOffCurrentFrame(index);
		setWrite.dstBinding = GetBinding();
		setWrite.dstArrayElement = 0;
		setWrite.descriptorCount = m_CreateInfo.ArraySize == 0 ? 1 : m_CreateInfo.ArraySize;
		setWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_Buffers[index];
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
	}

	void VulkanUniformBuffer::DestroyHandle() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VulkanAllocator& allocator = VulkanAllocator::Get();
		for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
			vmaDestroyBuffer(allocator.GetVmaInstance(), m_Buffers[i], m_BufferVma[i]);
	}

	VulkanUniformImageBuffer::VulkanUniformImageBuffer(UniformBufferCreateInfo& createInfo)
		: UniformBuffer(createInfo), m_DescriptorSet(createInfo.InternalInfo.As<VulkanRHIUniformCreateInfo>()->DescriptorSet) {
	}

	void VulkanUniformImageBuffer::BindImage(Ref<VulkanImage2D> image) {
		m_Images.push_back(image);
	}

	void VulkanUniformImageBuffer::Update() {
		if (m_Images.size() == 0)
			return;

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		const uint32_t index = swapChain.GetCurrentFrameIndex();
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		VkWriteDescriptorSet setWrite{};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.dstSet = m_DescriptorSet->GetSetBasedOffCurrentFrame(index);
		setWrite.dstBinding = GetBinding();
		setWrite.dstArrayElement = 0;
		setWrite.descriptorCount = m_Images.size();
		setWrite.pTexelBufferView = nullptr;

		std::vector<VkDescriptorImageInfo> imageInfos;

		for (uint32_t i = 0; i < m_Images.size(); i++) {
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = m_Images[i]->GetCurrentLayout();
			imageInfo.imageView = m_Images[i]->GetImageView().GetVulkanHandle();
			imageInfo.sampler = m_Images[i]->GetImageView().GetSampler();

			imageInfos.push_back(imageInfo);
		}

		setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setWrite.pImageInfo = imageInfos.data();

		vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);

		m_Images.clear();
	}

	void VulkanUniformImageBuffer::DestroyHandle() {
		//Empty, images are being deleted in the material class
	}
}