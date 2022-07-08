#pragma once

#include "../UniformBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/VulkanDescriptors.h"
#include "Renderer/Image/VulkanImage.h"

namespace Lucy {

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(UniformBufferCreateInfo& createInfo);
		virtual ~VulkanUniformBuffer() = default;

		void SetData(void* data, uint32_t size, uint32_t offset);
		void Update() override;
		void DestroyHandle() override;

		inline Ref<VulkanDescriptorSet> GetDescriptorSet() noexcept { return m_DescriptorSet; }
	private:
		Ref<VulkanDescriptorSet> m_DescriptorSet = nullptr;

		std::vector<VkBuffer> m_Buffers;
		std::vector<VmaAllocation> m_BufferVma;
	};

	class VulkanUniformImageBuffer : public UniformBuffer {
	public:
		VulkanUniformImageBuffer(UniformBufferCreateInfo& createInfo);
		virtual ~VulkanUniformImageBuffer() = default;

		void BindImage(Ref<VulkanImage2D> image);
		void Update() override;
		void DestroyHandle() override;

		inline Ref<VulkanDescriptorSet> GetDescriptorSet() noexcept { return m_DescriptorSet; }
	private:
		std::vector<Ref<VulkanImage2D>> m_Images;

		Ref<VulkanDescriptorSet> m_DescriptorSet = nullptr;
	};
}