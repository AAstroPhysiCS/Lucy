#include "lypch.h"
#include "OpenGLRenderCommand.h"

#include "glad/glad.h"
#include "RenderPass.h"

#include "Context/OpenGLPipeline.h"

namespace Lucy {

	void OpenGLRenderCommand::Begin(RefLucy<Pipeline> pipeline) {
		if (s_ActivePipeline) LUCY_ASSERT(false);

		auto& renderPass = pipeline->GetRenderPass();

		RenderPassBeginInfo info;
		renderPass->Begin(info);
		pipeline->GetFrameBuffer()->Bind();
		s_ActivePipeline = pipeline;

		Rasterization rasterization = pipeline->GetRasterization();
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

		auto& frameBuffer = pipeline->GetFrameBuffer();
		auto [r, g, b, a] = renderPass->GetClearColor();

		if (frameBuffer->GetBlitted())
			frameBuffer->Blit();
		ClearColor(r, g, b, a);
		Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRenderCommand::End(RefLucy<Pipeline> pipeline) {
		auto& renderPass = As(pipeline, OpenGLPipeline)->GetRenderPass();

		//reverting the changes back
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glLineWidth(1.0f);
		glDisable(GL_CULL_FACE);
		RenderPassEndInfo info;
		renderPass->End(info);
		pipeline->GetFrameBuffer()->Unbind();

		s_ActivePipeline = nullptr;
	}

	GLenum GetGLMode(Topology topology) {
		switch (topology) {
			case Topology::LINES:
				return GL_LINES;
				break;
			case Topology::POINTS:
				return GL_POINTS;
				break;
			case Topology::TRIANGLES:
				return GL_TRIANGLES;
				break;
		}
	}

	void OpenGLRenderCommand::ClearColor(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
	}

	void OpenGLRenderCommand::Clear(uint32_t bitField) {
		glClear(bitField);
	}

	void OpenGLRenderCommand::ReadBuffer(uint32_t mode) {
		glReadBuffer(mode);
	}

	void OpenGLRenderCommand::ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput) {
		glReadPixels(x, y, width, height, GL_RGB, GL_FLOAT, pixelValueOutput);
	}

	void OpenGLRenderCommand::ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode) {
		glNamedFramebufferReadBuffer(frameBuffer->GetID(), mode);
	}

	void OpenGLRenderCommand::DrawElements(uint32_t count, uint32_t indices) {
		if (!s_ActivePipeline) LUCY_ASSERT(false);
		glDrawElements(GetGLMode(s_ActivePipeline->GetTopology()), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)));
	}

	void OpenGLRenderCommand::DrawElementsBaseVertex(uint32_t count, uint32_t indices, int32_t basevertex) {
		if (!s_ActivePipeline) LUCY_ASSERT(false);
		glDrawElementsBaseVertex(GetGLMode(s_ActivePipeline->GetTopology()), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)), basevertex);
	}
}
