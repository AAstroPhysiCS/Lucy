#pragma once

#include "Pipeline.h"
#include "../Buffer/OpenGL/OpenGLVertexBuffer.h"

#include "../RenderPass.h"

namespace Lucy {

	class OpenGLPipeline : public Pipeline {
	public:
		OpenGLPipeline(PipelineSpecification& specs);
		virtual ~OpenGLPipeline() = default;

		friend class Mesh;
	private:
		void UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer);

		uint32_t GetTypeFromSize(ShaderDataSize size);
		uint32_t CalculateStride();
	};
}
