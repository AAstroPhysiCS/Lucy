#pragma once

#include "Renderer/Memory/Buffer/UniformBuffer.h"
#include "Renderer/Memory/Buffer/SharedStorageBuffer.h"

namespace Lucy {

	struct DescriptorSetCreateInfo {
		uint32_t SetIndex = 0;
		bool Bindless = false;
		Ref<void> InternalInfo = nullptr;
	};

	class DescriptorSet {
	public:
		DescriptorSet(const DescriptorSetCreateInfo& createInfo);
		virtual ~DescriptorSet() = default;
		
		virtual void Update() = 0;

		void AddBuffer(const Ref<UniformBuffer>& buffer);
		void AddBuffer(const Ref<SharedStorageBuffer>& buffer);
		void Destroy();

		inline const std::vector<Ref<UniformBuffer>>& GetAllUniformBuffers() const { return m_UniformBuffers; }
		inline const std::vector<Ref<SharedStorageBuffer>>& GetAllSharedStorageBuffers() const { return m_SharedStorageBuffers; }

		static Ref<DescriptorSet> Create(const DescriptorSetCreateInfo& createInfo);
	protected:
		DescriptorSetCreateInfo m_CreateInfo;

		std::vector<Ref<UniformBuffer>> m_UniformBuffers;
		std::vector<Ref<SharedStorageBuffer>> m_SharedStorageBuffers;
	};
}

