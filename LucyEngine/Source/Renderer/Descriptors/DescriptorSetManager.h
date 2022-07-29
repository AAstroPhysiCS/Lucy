#pragma once

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class Shader;

	class DescriptorSetManager final {
	public:
		static void Init();
		static void LoadDescriptorSets(Ref<Shader> shader);

		DescriptorSetManager() = delete;
		virtual ~DescriptorSetManager() = delete;

	private:
		//TODO: abstract this
		inline static Ref<VulkanDescriptorPool> s_DescriptorPool = nullptr;

		inline static std::vector<VkDescriptorPoolSize> s_PoolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }
		};
	};
}

