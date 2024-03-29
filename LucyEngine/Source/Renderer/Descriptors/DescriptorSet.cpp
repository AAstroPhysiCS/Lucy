#include "lypch.h"
#include "DescriptorSet.h"
#include "VulkanDescriptorSet.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<DescriptorSet> DescriptorSet::Create(const DescriptorSetCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
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
