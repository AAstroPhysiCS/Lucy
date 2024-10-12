#pragma once

#include "Renderer/Shader/ComputeShader.h"

namespace Lucy {

	struct ComputePipelineCreateInfo {
		Ref<ComputeShader> Shader = nullptr;
	};

	class ComputePipeline : public RenderResource {
	public:
		ComputePipeline(const ComputePipelineCreateInfo& createInfo) 
			: RenderResource("ComputePipeline"), m_CreateInfo(createInfo) {
		}
		virtual ~ComputePipeline() = default;

		inline const Ref<ComputeShader>& GetShader() const { return m_CreateInfo.Shader; }

		virtual void RTBind(void* commandBufferHandle) = 0;
		virtual void RTRecreate() = 0;
		virtual void RTDispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
	protected:
		ComputePipelineCreateInfo m_CreateInfo;
	};
}