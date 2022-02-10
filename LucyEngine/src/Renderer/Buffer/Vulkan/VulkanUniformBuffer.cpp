#include "lypch.h"
#include "VulkanUniformBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanRenderer.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Buffer/Vulkan/VulkanBufferUtils.h"

namespace Lucy {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding) {
		Renderer::Submit([this, size, binding]() {

			/*
			//TODO: CHANGE THIS
			//I need to change this because what if i dont wanna have just one binding and multiple?
			//Make a parameter in the specification or a new specification all together and put the layout bindings into that.
			VkDescriptorSetLayoutBinding uniformBinding{};
			uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBinding.binding = binding;
			uniformBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
			uniformBinding.descriptorCount = 1;

			VulkanDescriptorSetSpecifications setSpecs;
			setSpecs.LayoutBindings = { uniformBinding };
			setSpecs.Pool = s_DescriptorPool;
			m_DescriptorSet = CreateRef<VulkanDescriptorSet>(setSpecs);
			*/

			const uint32_t instanceSize = VulkanRenderer::MAX_FRAMES_IN_FLIGHT;
			VulkanBufferUtils::CreateVulkanBuffer(size * instanceSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
												  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_Buffer, m_BufferMemory);
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
		vkDestroyBuffer(device, m_Buffer, nullptr);
		vkFreeMemory(device, m_BufferMemory, nullptr);
	}
}