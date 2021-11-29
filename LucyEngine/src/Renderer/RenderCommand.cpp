#include "lypch.h"

#include "RenderPass.h"
#include "Context/RendererAPI.h"
#include "RenderCommand.h"

namespace Lucy {

	RenderPass* RenderCommand::s_ActiveRenderPass = nullptr;

	void RenderCommand::Begin(RefLucy<RenderPass> renderPass) {
		if (s_ActiveRenderPass) LUCY_ASSERT(false);
		RenderPass::Begin(renderPass);
		s_ActiveRenderPass = renderPass.get();

		auto& frameBuffer = renderPass->GetFrameBuffer();
		auto [r, g, b, a] = renderPass->GetClearColor();

		if (frameBuffer->GetBlitted())
			frameBuffer->Blit();
		RenderCommand::ClearColor(r, g, b, a);
		RenderCommand::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void RenderCommand::End(RefLucy<RenderPass> renderPass) {
		RenderPass::End(renderPass);
		s_ActiveRenderPass = nullptr;
	}

	void RenderCommand::ClearColor(float r, float g, float b, float a) {
		Renderer::s_RendererAPI->ClearColor(r, g, b, a);
	}

	void RenderCommand::Clear(uint32_t bitField) {
		Renderer::s_RendererAPI->Clear(bitField);
	}

	void RenderCommand::ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput) {
		Renderer::s_RendererAPI->ReadPixels(x, y, width, height, pixelValueOutput);
	}

	void RenderCommand::ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode) {
		Renderer::s_RendererAPI->ReadBuffer(frameBuffer, mode);
	}

	void RenderCommand::ReadBuffer(uint32_t mode) {
		Renderer::s_RendererAPI->ReadBuffer(mode);
	}

	GLenum GetGLMode(Topology topology) {
		if (Renderer::GetCurrentRenderAPI() == RenderAPI::OpenGL) {
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
	}

	void RenderCommand::DrawElements(uint32_t count, uint32_t indices) {
		if (!s_ActiveRenderPass) LUCY_ASSERT(false);
		if (Renderer::GetCurrentRenderAPI() == RenderAPI::OpenGL)
			Renderer::s_RendererAPI->DrawElements(GetGLMode(s_ActiveRenderPass->GetPipeline()->GetTopology()), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)));
	}

	void RenderCommand::DrawElementsBaseVertex(uint32_t count, uint32_t indices, int32_t basevertex) {
		if (!s_ActiveRenderPass) LUCY_ASSERT(false);
		if (Renderer::GetCurrentRenderAPI() == RenderAPI::OpenGL)
			Renderer::s_RendererAPI->DrawElementsBaseVertex(GetGLMode(s_ActiveRenderPass->GetPipeline()->GetTopology()), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)), basevertex);
	}

	void RenderCommand::SwapBuffers(GLFWwindow* window) {
		Renderer::s_RendererAPI->SwapBuffers(window);
	}
}