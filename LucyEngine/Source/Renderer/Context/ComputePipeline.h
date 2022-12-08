#pragma once

#include "ContextPipeline.h"

#include "Renderer/Shader/ComputeShader.h"

namespace Lucy {

	struct ComputePipelineCreateInfo {
		Ref<ComputeShader> Shader = nullptr;
	};

	class ComputePipeline : public ContextPipeline {
	public:
		ComputePipeline(const ComputePipelineCreateInfo& createInfo);
		virtual ~ComputePipeline() = default;

		inline const Ref<ComputeShader>& GetComputeShader() const { return m_CreateInfo.Shader; }

		virtual void Dispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
	protected:
		ComputePipelineCreateInfo m_CreateInfo;
	};
}