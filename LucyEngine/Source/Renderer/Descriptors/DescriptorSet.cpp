#include "lypch.h"
#include "DescriptorSet.h"

#include "Renderer/Renderer.h"

#include "VulkanDescriptorSet.h"

namespace Lucy {

	Ref<DescriptorSet> DescriptorSet::Create(const DescriptorSetCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				LUCY_ASSERT(false);
				//return Memory::CreateRef<OpenGLDescriptorSet>(createInfo);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanDescriptorSet>(createInfo);
				break;
		}
		return nullptr;
	}

	DescriptorSet::DescriptorSet(const DescriptorSetCreateInfo& createInfo) 
		: m_CreateInfo(createInfo) {
	}

	void DescriptorSet::AddBuffer(const Ref<UniformBuffer>& buffer) {
		m_UniformBuffers.push_back(buffer);
	}

	void DescriptorSet::AddBuffer(const Ref<SharedStorageBuffer>& buffer) {
		m_SharedStorageBuffers.push_back(buffer);
	}

	void DescriptorSet::Destroy() {
		for (Ref<UniformBuffer>& buffer : m_UniformBuffers)
			buffer->DestroyHandle();

		for (Ref<SharedStorageBuffer>& buffer : m_SharedStorageBuffers)
			buffer->DestroyHandle();
	}
}
