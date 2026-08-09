#pragma once

#include "Pipeline.h"

#include "Renderer/Shader/ComputeShader.h"

namespace Lucy {

	struct ComputePipelineCreateInfo {

	};

	class ComputePipeline : public Pipeline {
	public:
		ComputePipeline(const ComputePipelineCreateInfo& createInfo, Ref<Shader> shader) 
			: Pipeline("ComputePipeline", shader) {
		}
		virtual ~ComputePipeline() = default;

		ComputePipeline(const ComputePipeline&) = delete;
		ComputePipeline& operator=(const ComputePipeline&) = delete;
		ComputePipeline(ComputePipeline&&) = delete;
		ComputePipeline& operator=(ComputePipeline&&) = delete;

		virtual void RTBind(void* commandBufferHandle) = 0;
		virtual void RTRecreate(Ref<Shader> newShader) = 0;
		virtual void RTDispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
	};
}