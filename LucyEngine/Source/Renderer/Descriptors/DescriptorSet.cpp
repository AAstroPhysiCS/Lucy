#include "lypch.h"
#include "DescriptorSet.h"
#include "VulkanDescriptorSet.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	DescriptorSet::DescriptorSet(const DescriptorSetCreateInfo& createInfo) 
		: RenderResource("DescriptorSet"), m_CreateInfo(createInfo) {
	}

	void DescriptorSet::AddUniformBuffer(const std::string& name, RenderResourceHandle bufferHandle) {
		m_UniformBufferHandles.try_emplace(name, bufferHandle);
	}

	void DescriptorSet::AddSharedStorageBuffer(const std::string& name, RenderResourceHandle bufferHandle) {
		m_SharedStorageBufferHandles.try_emplace(name, bufferHandle);
	}
}
