#pragma once

#include "Renderer/Memory/Buffer/UniformBuffer.h"
#include "Renderer/Memory/Buffer/SharedStorageBuffer.h"

namespace Lucy {

	constexpr uint32_t MAX_DYNAMIC_DESCRIPTOR_COUNT = 1024u;
	constexpr uint32_t MAX_DYNAMICALLY_ALLOCATED_BUFFER_SIZE = MAX_DYNAMIC_DESCRIPTOR_COUNT * 10u; //10 kilobytes

	struct DescriptorSetCreateInfo {
		uint32_t SetIndex = 0;
		bool Bindless = false;
		Ref<void> InternalInfo = nullptr;
	};

	class DescriptorSet {
	public:
		static Ref<DescriptorSet> Create(const DescriptorSetCreateInfo& createInfo);

		DescriptorSet(const DescriptorSetCreateInfo& createInfo);
		virtual ~DescriptorSet() = default;
		
		virtual void Update() = 0;

		void AddBuffer(const Ref<UniformBuffer>& buffer);
		void AddBuffer(const Ref<SharedStorageBuffer>& buffer);
		void Destroy();

		inline uint32_t GetSetIndex() { return m_CreateInfo.SetIndex; }

		inline const std::vector<Ref<UniformBuffer>>& GetAllUniformBuffers() const { return m_UniformBuffers; }
		inline const std::vector<Ref<SharedStorageBuffer>>& GetAllSharedStorageBuffers() const { return m_SharedStorageBuffers; }
	protected:
		DescriptorSetCreateInfo m_CreateInfo;

		std::vector<Ref<UniformBuffer>> m_UniformBuffers;
		std::vector<Ref<SharedStorageBuffer>> m_SharedStorageBuffers;
	};
}

