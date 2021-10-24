#include "RenderPass.h"
#include "Context/RendererAPI.h"
#include "RenderCommand.h"

namespace Lucy {

	void RenderCommand::Begin(RefLucy<RenderPass> renderPass)
	{
		RenderPass::Begin(renderPass);
	}

	void RenderCommand::End(RefLucy<RenderPass> renderPass)
	{
		RenderPass::End(renderPass);
	}

	void RenderCommand::ClearColor(float r, float g, float b, float a)
	{
		Renderer::m_RendererAPI->ClearColor(r, g, b, a);
	}

	void RenderCommand::Clear(uint32_t bitField)
	{
		Renderer::m_RendererAPI->Clear(bitField);
	}

	void RenderCommand::SwapBuffers(GLFWwindow* window)
	{
		Renderer::m_RendererAPI->SwapBuffers(window);
	}
}