#include "lypch.h"
#include "GraphicsPipeline.h"

namespace Lucy {

	GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
		: ContextPipeline(ContextPipelineType::Graphics), m_CreateInfo(createInfo) {
	}

	uint32_t GraphicsPipeline::GetSizeFromType(ShaderDataSize size) {
		switch (size) {
			case ShaderDataSize::Int1:
			case ShaderDataSize::Float1: return 1; break;
			case ShaderDataSize::Int2:
			case ShaderDataSize::Float2: return 2; break;
			case ShaderDataSize::Int3:
			case ShaderDataSize::Float3: return 3; break;
			case ShaderDataSize::Int4:
			case ShaderDataSize::Float4: return 4; break;
			case ShaderDataSize::Mat4:	 return 4 * 4; break;
		}
		return 0;
	}

	uint32_t GraphicsPipeline::CalculateStride(VertexShaderLayout vertexLayout) {
		uint32_t stride = 0;
		for (const auto& [name, size] : vertexLayout) {
			stride += GetSizeFromType(size);
		}
		return stride;
	}
}