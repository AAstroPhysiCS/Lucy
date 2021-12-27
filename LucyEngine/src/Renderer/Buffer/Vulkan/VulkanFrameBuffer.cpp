#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "../../Context/VulkanDevice.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs) {
		auto& swapChainInstance = VulkanSwapChain::Get();
		size_t size = swapChainInstance.m_SwapChainImageViews.size();
		m_SwapChainFrameBuffers.resize(size);

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		for (uint32_t i = 0; i < size; i++) {
			VkFramebufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.attachmentCount = 1;
			createInfo.renderPass = m_Specs.RenderPass->GetVulkanHandle();
			createInfo.pAttachments = &swapChainInstance.m_SwapChainImageViews[i];
			createInfo.width = swapChainInstance.GetExtent().width;
			createInfo.height = swapChainInstance.GetExtent().height;
			createInfo.layers = 1;

			LUCY_VULKAN_ASSERT(vkCreateFramebuffer(device, &createInfo, nullptr, &m_SwapChainFrameBuffers[i]));
		}
	}

	void VulkanFrameBuffer::Bind() {

	}

	void VulkanFrameBuffer::Unbind() {

	}

	void VulkanFrameBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		for (uint32_t i = 0; i < m_SwapChainFrameBuffers.size(); i++) {
			vkDestroyFramebuffer(device, m_SwapChainFrameBuffers[i], nullptr);
		}
	}

	void VulkanFrameBuffer::Blit() {

	}

	void VulkanFrameBuffer::Resize(int32_t width, int32_t height) {

	}
}