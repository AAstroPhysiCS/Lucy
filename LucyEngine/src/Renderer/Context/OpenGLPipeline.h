#pragma once

#include "Pipeline.h"
#include "../Buffer/OpenGL/OpenGLVertexBuffer.h"

#include "../RenderPass.h"

namespace Lucy {

	class OpenGLPipeline : public Pipeline {
	public:
		OpenGLPipeline(PipelineSpecification& specs);
		virtual ~OpenGLPipeline() = default;

		void BeginVirtual();
		void EndVirtual();

		friend class Mesh;
	private:
		void UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer);
	};
}
