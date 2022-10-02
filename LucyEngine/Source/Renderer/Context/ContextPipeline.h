#pragma once

#include "Renderer/Shader/Shader.h"
#include "Renderer/Descriptors/DescriptorSet.h"
#include "Renderer/Memory/Buffer/PushConstant.h"

namespace Lucy {

	struct ComputePipelineCreateInfo;
	struct GraphicsPipelineCreateInfo;

	class ComputeShader;

	class VulkanPushConstant;
	class DescriptorSet;
	class VulkanDescriptorPool;
	class Shader;

	class VulkanContextPipelineUtils {
		static void ParseVulkanDescriptorSets(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const Ref<Shader>& shader,
											  Ref<VulkanDescriptorPool>& descriptorPool, std::vector<Ref<DescriptorSet>>& descriptorSets,
											  std::vector<VulkanPushConstant>& pushConstants);

		friend class VulkanGraphicsPipeline;
		friend class VulkanComputePipeline;
	};

	enum class ContextPipelineType : uint8_t {
		Graphics,
		Compute
	};

	class ContextPipeline {
	public:
		static Ref<ContextPipeline> Create(const GraphicsPipelineCreateInfo& createInfo);
		static Ref<ContextPipeline> Create(const ComputePipelineCreateInfo& createInfo);

		ContextPipeline(ContextPipelineType type);
		virtual ~ContextPipeline() = default;

		inline const std::vector<Ref<DescriptorSet>>& GetDescriptorSets() const { return m_DescriptorSets; }
		inline const ContextPipelineType GetType() const { return m_Type; }

		template <class T>
		inline Ref<T> GetUniformBuffers(const char* name) {
			for (Ref<DescriptorSet> set : m_DescriptorSets) {
				const auto& uniformBuffers = set->GetAllUniformBuffers();
				for (const Ref<UniformBuffer>& ubo : uniformBuffers) {
					if (name == ubo->GetName()) {
						return ubo.As<T>();
					}
				}
			}
			LUCY_CRITICAL(fmt::format("Could not find a suitable Uniform Buffer for the given name: {0}", name));
			LUCY_ASSERT(false);
			return nullptr;
		}

		template <class T>
		inline Ref<T> GetSharedStorageBuffers(const char* name) {
			for (Ref<DescriptorSet> set : m_DescriptorSets) {
				const auto& ssbos = set->GetAllSharedStorageBuffers();
				for (const Ref<SharedStorageBuffer>& ssbo : ssbos) {
					if (name == ssbo->GetName()) {
						return ssbo.As<T>();
					}
				}
			}
			LUCY_CRITICAL(fmt::format("Could not find a suitable Uniform Buffer for the given name: {0}", name));
			LUCY_ASSERT(false);
			return nullptr;
		}

		VulkanPushConstant& GetPushConstants(const char* name);

		virtual void Bind(void* commandBufferHandle) = 0;
		virtual void Recreate(uint32_t width, uint32_t height) = 0;
		virtual void Destroy() = 0;
	protected:
		std::vector<Ref<DescriptorSet>> m_DescriptorSets;
		std::vector<VulkanPushConstant> m_PushConstants;

		ContextPipelineType m_Type;
	};
}