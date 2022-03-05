#pragma once

#include "Pipeline.h"
#include "../Buffer/OpenGL/OpenGLVertexBuffer.h"

#include "../RenderPass.h"

namespace Lucy {

	class OpenGLPipeline : public Pipeline {
	public:
		explicit OpenGLPipeline(const PipelineSpecification& specs);
		virtual ~OpenGLPipeline() = default;

		void BeginVirtual() override;
		void EndVirtual() override;

		friend class Mesh;
	private:
		void UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer);
	};
}
