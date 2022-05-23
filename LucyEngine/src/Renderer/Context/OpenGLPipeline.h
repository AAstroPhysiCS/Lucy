#pragma once

#include "Pipeline.h"
#include "../Buffer/OpenGL/OpenGLVertexBuffer.h"
#include "../Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "../RenderPass.h"

namespace Lucy {

	class OpenGLPipeline : public Pipeline {
	public:
		OpenGLPipeline(const PipelineSpecification& specs);
		virtual ~OpenGLPipeline() = default;

		void Bind(PipelineBindInfo bindInfo) override;
		void Unbind();
		void Destroy() override;
		void UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer);
	private:
		void ParseUniformBuffers();
	};
}
