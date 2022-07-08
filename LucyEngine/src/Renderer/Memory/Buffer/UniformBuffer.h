#pragma once
#include <optional>
#include "Buffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanDescriptorSet;

	struct ShaderMemberVariable;

	struct VulkanRHIUniformCreateInfo {
		Ref<VulkanDescriptorSet> DescriptorSet = nullptr;
		VkDescriptorType Type;
	};

	struct UniformBufferCreateInfo {
		std::string Name = "Unnamed Uniform Buffer";
		uint32_t BufferSize = 0;
		uint32_t Binding = 0;
		Ref<void> InternalInfo = nullptr; //to be implemented by RHI's
		std::vector<ShaderMemberVariable> ShaderMemberVariables;
		uint32_t ArraySize = 0; //default is 0, which means no array
	};

	class UniformBuffer : public Buffer<void*> {
	public:
		virtual ~UniformBuffer() = default;

		static Ref<UniformBuffer> Create(UniformBufferCreateInfo& createInfo);
		
		virtual void Update() = 0;
		virtual void DestroyHandle() = 0;

		inline std::string GetName() const { return m_CreateInfo.Name; }
		inline uint32_t GetBinding() const { return m_CreateInfo.Binding; }
		inline uint32_t GetSize() const { return m_CreateInfo.BufferSize; }
	protected:
		UniformBuffer(UniformBufferCreateInfo& createInfo);

		UniformBufferCreateInfo m_CreateInfo;
	};
}