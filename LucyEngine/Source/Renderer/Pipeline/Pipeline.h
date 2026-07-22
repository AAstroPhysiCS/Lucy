#pragma once

#include "Renderer/Device/RenderDeviceResource.h"

#include "Renderer/Memory/Buffer/PushConstant.h"

namespace Lucy {

	class Shader;
	struct ShaderVariable;

	class Pipeline : public RenderDeviceResource {
	public:
		Pipeline(std::string_view name, Ref<Shader> shader);
		virtual ~Pipeline() = default;

		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline(Pipeline&&) = delete;
		Pipeline& operator=(Pipeline&&) = delete;

		inline const Ref<Shader>& GetShader() const { return m_Shader; }
		PipelineConstant& GetPipelineConstants(const std::string& name);
	protected:
		virtual void RTDestroyResource() override;

		inline const std::vector<PipelineConstant>& GetPipelineConstants() const { return m_PushConstants; }

		void AddPushConstant(const ShaderVariable& pc);
	private:
		std::vector<PipelineConstant> m_PushConstants;

		Ref<Shader> m_Shader = nullptr;
	};
}