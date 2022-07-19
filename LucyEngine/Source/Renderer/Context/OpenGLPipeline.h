#pragma once

#include "Pipeline.h"
#include  "../Memory/Buffer/OpenGL/OpenGLVertexBuffer.h"
#include  "../Memory/Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "../RenderPass.h"

namespace Lucy {

	class OpenGLPipeline : public Pipeline {
	public:
		OpenGLPipeline(const PipelineCreateInfo& createInfo);
		virtual ~OpenGLPipeline() = default;

		void Bind(PipelineBindInfo bindInfo) override;
		void Unbind();
		void Destroy() override;
		void UploadVertexLayout(Ref<VertexBuffer>& vertexBuffer);
	private:
		void ParseBuffers();
	};
}
