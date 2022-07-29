#include "lypch.h"
#include "DescriptorSetManager.h"

namespace Lucy {

	void DescriptorSetManager::Init() {
		VulkanDescriptorPoolCreateInfo poolCreateInfo;
		poolCreateInfo.PoolSizesVector = s_PoolSizes;
		poolCreateInfo.MaxSet = 100;
		poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

		//s_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);
	}

	void DescriptorSetManager::LoadDescriptorSets(Ref<Shader> shader) {

	}
}