#pragma once

#include "Renderer/Descriptors/DescriptorSet.h"
#include "Renderer/Memory/Buffer/PushConstant.h"

namespace Lucy {

	class ComputeShader;

	class ComputePipeline {
	public:
		static Ref<ComputePipeline> Create(const Ref<ComputeShader>& shader);

		ComputePipeline(const Ref<ComputeShader>& shader);
		virtual ~ComputePipeline() = default;

		inline const Ref<ComputeShader>& GetComputeShader() { return m_ComputeShader; }
		inline const std::vector<Ref<DescriptorSet>>& GetDescriptorSets() const { return m_DescriptorSets; }

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
		virtual void Dispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void Destroy() = 0;
	protected:
		virtual void ParseDescriptorSets() = 0;

		Ref<ComputeShader> m_ComputeShader = nullptr;

		std::vector<Ref<DescriptorSet>> m_DescriptorSets;
		std::vector<VulkanPushConstant> m_PushConstants;
	};
}