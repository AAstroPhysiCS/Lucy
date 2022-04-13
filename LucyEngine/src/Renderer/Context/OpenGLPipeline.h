#pragma once

#include "Pipeline.h"
#include "../Buffer/OpenGL/OpenGLVertexBuffer.h"

#include "../RenderPass.h"

namespace Lucy {

	class OpenGLPipeline : public Pipeline {
	public:
		OpenGLPipeline(const PipelineSpecification& specs);
		virtual ~OpenGLPipeline() = default;

		void BeginVirtual() override;
		void EndVirtual() override;
		void Recreate() override;
		void Destroy() override;

		friend class OpenGLMesh;
	private:
		void UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer);
		void ParseUniformBuffers();
	};
}
