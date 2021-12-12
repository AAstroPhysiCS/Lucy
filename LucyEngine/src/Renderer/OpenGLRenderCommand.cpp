#include "lypch.h"
#include "OpenGLRenderCommand.h"

#include "glad/glad.h"
#include "RenderPass.h"

namespace Lucy {

	void OpenGLRenderCommand::Begin(RefLucy<RenderPass> renderPass) {
		if (s_ActiveRenderPass) LUCY_ASSERT(false);
		renderPass->Begin();
		s_ActiveRenderPass = renderPass.get();

		auto& frameBuffer = renderPass->GetFrameBuffer();
		auto [r, g, b, a] = renderPass->GetClearColor();

		if (frameBuffer->GetBlitted())
			frameBuffer->Blit();
		ClearColor(r, g, b, a);
		Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRenderCommand::End(RefLucy<RenderPass> renderPass) {
		renderPass->End();
		s_ActiveRenderPass = nullptr;
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
		if (!s_ActiveRenderPass) LUCY_ASSERT(false);
		glDrawElements(GetGLMode(s_ActiveRenderPass->GetPipeline()->GetTopology()), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)));
	}

	void OpenGLRenderCommand::DrawElementsBaseVertex(uint32_t count, uint32_t indices, int32_t basevertex) {
		if (!s_ActiveRenderPass) LUCY_ASSERT(false);
		glDrawElementsBaseVertex(GetGLMode(s_ActiveRenderPass->GetPipeline()->GetTopology()), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)), basevertex);
	}
}
