#include "lypch.h"
#include "VulkanUniformBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Buffer/Vulkan/VulkanBufferUtils.h"

namespace Lucy {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding) {
		Renderer::Submit([this, size, binding]() {
			VkDescriptorSetLayoutBinding bindingInfo{};
			bindingInfo.binding = binding;
			bindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindingInfo.descriptorCount = 1;
			bindingInfo.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
			bindingInfo.pImmutableSamplers = nullptr; //not relevant since this is only a uniform buffer

			VkDescriptorSetLayoutCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.bindingCount = 1;
			createInfo.pBindings = &bindingInfo;

			LUCY_VULKAN_ASSERT(vkCreateDescriptorSetLayout(VulkanDevice::Get().GetLogicalDevice(), &createInfo, nullptr, &m_Layout));

			VulkanBufferUtils::CreateVulkanBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, 
												  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_BufferMemory);
		});
	}

	void VulkanUniformBuffer::Bind() {

	}

	void VulkanUniformBuffer::Unbind() {

	}

	void VulkanUniformBuffer::SetData(void* data, uint32_t size, uint32_t offset) {
		Renderer::Submit([this, data, size, offset]() {
			VkDevice device = VulkanDevice::Get().GetLogicalDevice();
			void* dataLocal;
			vkMapMemory(device, m_BufferMemory, offset, size, 0, &dataLocal);
			memcpy(dataLocal, data, size);
			vkUnmapMemory(device, m_BufferMemory);
		});
	}

	void VulkanUniformBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDestroyDescriptorSetLayout(device, m_Layout, nullptr);
		vkDestroyBuffer(device, m_Buffer, nullptr);
		vkFreeMemory(device, m_BufferMemory, nullptr);
	}
}