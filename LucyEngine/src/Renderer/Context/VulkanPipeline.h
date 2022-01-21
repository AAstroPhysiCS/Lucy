#pragma once

#include "Pipeline.h"

namespace Lucy {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(PipelineSpecification& specs);
		virtual ~VulkanPipeline() = default;

		void Destroy();
		void Recreate(float sizeX, float sizeY);

		void BeginVirtual();
		void EndVirtual();

		inline VkPipeline GetVulkanHandle() { return m_Pipeline; }
	private:
		void Create();

		VkVertexInputBindingDescription CreateBindingDescriptor();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescriptor(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		VkPipeline m_Pipeline{};
		VkPipelineLayout m_PipelineLayout{};
	};
}