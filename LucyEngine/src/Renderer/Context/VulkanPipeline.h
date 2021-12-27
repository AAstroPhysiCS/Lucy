#pragma once

#include "Pipeline.h"

namespace Lucy {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(PipelineSpecification& specs);
		virtual ~VulkanPipeline() = default;

		void UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer);
		void Destroy();

		inline VkPipeline GetVulkanHandle() { return m_Pipeline; }
	private:
		void Create();

		VkPipeline m_Pipeline{};
		VkPipelineLayout m_PipelineLayout{};
	};
}