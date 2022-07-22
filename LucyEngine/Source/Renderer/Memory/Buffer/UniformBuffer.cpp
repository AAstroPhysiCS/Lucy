#include "lypch.h"

#include "Renderer/Renderer.h"

#include "Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	Ref<UniformBuffer> UniformBuffer::Create(UniformBufferCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				if (createInfo.Type == DescriptorType::SampledImage ||
					createInfo.Type == DescriptorType::Sampler ||
					createInfo.Type == DescriptorType::CombinedImageSampler) {
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