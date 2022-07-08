#include "lypch.h"

#include "Renderer/Renderer.h"

#include "OpenGL/OpenGLUniformBuffer.h"
#include "Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	Ref<UniformBuffer> UniformBuffer::Create(UniformBufferCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL: {
				return Memory::CreateRef<OpenGLUniformBuffer>(createInfo);
			}
			case RenderArchitecture::Vulkan: {
				auto& internalInfo = createInfo.InternalInfo.As<VulkanRHIUniformCreateInfo>();
				if (internalInfo->Type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
					internalInfo->Type == VK_DESCRIPTOR_TYPE_SAMPLER ||
					internalInfo->Type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
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
	}
}