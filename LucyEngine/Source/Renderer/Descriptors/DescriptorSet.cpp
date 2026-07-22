#include "lypch.h"
#include "DescriptorSet.h"

namespace Lucy {

	DescriptorSet::DescriptorSet(const DescriptorSetCreateInfo& createInfo) 
		: RenderDeviceResource("DescriptorSet"), m_CreateInfo(createInfo) {
	}

	void DescriptorSet::AddUniformBuffer(const std::string& name, RenderDeviceResourceHandle bufferHandle) {
		m_UniformBufferHandles.try_emplace(name, bufferHandle);
	}

	void DescriptorSet::AddSharedStorageBuffer(const std::string& name, RenderDeviceResourceHandle bufferHandle) {
		m_SharedStorageBufferHandles.try_emplace(name, bufferHandle);
	}
}
