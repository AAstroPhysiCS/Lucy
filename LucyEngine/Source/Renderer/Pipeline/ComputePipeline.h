#pragma once

#include "Pipeline.h"

#include "Renderer/Shader/ComputeShader.h"

namespace Lucy {

	struct ComputePipelineCreateInfo {
		Ref<ComputeShader> Shader = nullptr;
	};

	class ComputePipeline : public Pipeline {
	public:
		ComputePipeline(const ComputePipelineCreateInfo& createInfo) 
			: Pipeline("ComputePipeline", createInfo.Shader), m_CreateInfo(createInfo) {
		}
		virtual ~ComputePipeline() = default;

		virtual void RTBind(void* commandBufferHandle) = 0;
		virtual void RTRecreate() = 0;
		virtual void RTDispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
	protected:
		ComputePipelineCreateInfo m_CreateInfo;
	};
}