#include "lypch.h"
#include "Vulkan/VulkanUniformBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<UniformBuffer> UniformBuffer::Create(UniformBufferCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				if (createInfo.Type == DescriptorType::SampledImage ||
					createInfo.Type == DescriptorType::Sampler ||
					createInfo.Type == DescriptorType::CombinedImageSampler ||
					createInfo.Type == DescriptorType::StorageImage) {
					return Memory::CreateRef<VulkanUniformImageBuffer>(createInfo);
				} else {
					return Memory::CreateRef<VulkanUniformBuffer>(createInfo);
				}
			}
		}
		return nullptr;
	}

	UniformBuffer::UniformBuffer(UniformBufferCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		Reserve(createInfo.BufferSize);
	}
}