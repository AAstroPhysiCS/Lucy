#pragma once

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

#if USE_INTEGRATED_GRAPHICS
	constexpr uint32_t MAX_DYNAMIC_DESCRIPTOR_COUNT = 32u;
	constexpr uint32_t MAX_DYNAMICALLY_ALLOCATED_BUFFER_SIZE = 1024u * 10u;
#else
	constexpr uint32_t MAX_DYNAMIC_DESCRIPTOR_COUNT = 1024u;
	constexpr uint32_t MAX_DYNAMICALLY_ALLOCATED_BUFFER_SIZE = MAX_DYNAMIC_DESCRIPTOR_COUNT * 10u; //10 kilobytes
#endif

	struct DescriptorSetCreateInfo {
		uint32_t SetIndex = 0;
		std::vector<ShaderUniformBlock> ShaderUniformBlocks;
	};

	class DescriptorSet : public RenderResource {
	public:
		DescriptorSet(const DescriptorSetCreateInfo& createInfo);
		virtual ~DescriptorSet() = default;
		
		virtual void RTUpdate() = 0;

		void AddUniformBuffer(const std::string& name, RenderResourceHandle bufferHandle);
		void AddSharedStorageBuffer(const std::string& name, RenderResourceHandle bufferHandle);

		inline uint32_t GetSetIndex() const { return m_CreateInfo.SetIndex; }

		inline const auto& GetAllUniformBufferHandles() const { return m_UniformBufferHandles; }
		inline const auto& GetAllSharedStorageBufferHandles() const { return m_SharedStorageBufferHandles; }
	protected:
		virtual void RTDestroyResource() = 0;

		DescriptorSetCreateInfo m_CreateInfo;
	private:
		std::unordered_map<std::string, RenderResourceHandle> m_UniformBufferHandles;
		std::unordered_map<std::string, RenderResourceHandle> m_SharedStorageBufferHandles;
	};
}

