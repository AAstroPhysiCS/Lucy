#include "lypch.h"

#include "OpenGLPipeline.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLPipeline::OpenGLPipeline(PipelineSpecification& specs)
		: Pipeline(specs) {
	}

	void OpenGLPipeline::BeginVirtual() {
		if (s_ActivePipeline) LUCY_ASSERT(false);

		auto& renderPass = GetRenderPass();

		RenderPassBeginInfo info;
		info.OpenGLFrameBuffer = As(GetFrameBuffer(), OpenGLFrameBuffer);
		renderPass->Begin(info);
		s_ActivePipeline = this;

		Rasterization rasterization = GetRasterization();
		switch (rasterization.PolygonMode) {
			case PolygonMode::FILL:
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case PolygonMode::LINE:
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;
			case PolygonMode::POINT:
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				break;
		}
		glLineWidth(rasterization.LineWidth);
		if (rasterization.DisableBackCulling)
			glDisable(GL_CULL_FACE);
		if (rasterization.CullingMode != 0) {
			glEnable(GL_CULL_FACE);
			glCullFace(rasterization.CullingMode);
		}

		auto& frameBuffer = GetFrameBuffer();
		auto [r, g, b, a] = renderPass->GetClearColor();

		if (frameBuffer->GetBlitted())
			frameBuffer->Blit();
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLPipeline::EndVirtual() {
		auto& renderPass = GetRenderPass();

		//reverting the changes back
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glLineWidth(1.0f);
		glDisable(GL_CULL_FACE);
		RenderPassEndInfo info;
		info.OpenGLFrameBuffer = As(GetFrameBuffer(), OpenGLFrameBuffer);
		renderPass->End(info);

		s_ActivePipeline = nullptr;
	}

	void OpenGLPipeline::UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer) {
		vertexBuffer->Bind({});

		uint32_t stride = CalculateStride(m_Specs.VertexShaderLayout);
		uint32_t offset = 0;
		uint32_t bufferIndex = 0;

		for (auto [name, size] : m_Specs.VertexShaderLayout.ElementList) {
			glEnableVertexAttribArray(bufferIndex);
			uint32_t apiSize = GetSizeFromType(size);
			//TODO: Ugly code
			switch (size) {
				case ShaderDataSize::Float1:
				case ShaderDataSize::Float2:
				case ShaderDataSize::Float3:
				case ShaderDataSize::Float4:
				case ShaderDataSize::Mat4:
					glVertexAttribPointer(bufferIndex, apiSize, GL_FLOAT, GL_FALSE, stride * sizeof(float), (const void*)(offset * sizeof(float)));
					break;
				case ShaderDataSize::Int1:
				case ShaderDataSize::Int2:
				case ShaderDataSize::Int3:
				case ShaderDataSize::Int4:
					glVertexAttribIPointer(bufferIndex, apiSize, GL_INT, stride * sizeof(float), (const void*)(offset * sizeof(float)));
					break;
			}

			offset += apiSize;
			bufferIndex++;
		}

		vertexBuffer->Unbind();
	}
}