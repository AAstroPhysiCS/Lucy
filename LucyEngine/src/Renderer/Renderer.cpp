#include "Renderer.h"

#include "Context/OpenGLRendererAPI.h"

namespace Lucy {

	RefLucy<RendererAPI> Renderer::m_RendererAPI;
	Scene Renderer::m_Scene;
	RefLucy<RenderContext> Renderer::m_RenderContext;
	RefLucy<FrameBuffer> Renderer::m_MainFrameBuffer;
	std::vector<RenderFunc> Renderer::m_RenderQueue;

	void Renderer::Init(RenderContextType renderContext)
	{
		m_RenderContext = RenderContext::Create(renderContext);
		m_RenderContext->PrintInfo();
		m_RendererAPI = RendererAPI::Create();

		//FrameBufferSpecification specs;
		//specs.multiSampled = true;

		//m_MainFrameBuffer = FrameBuffer::Create(specs);
	}

	inline void Renderer::Submit(const RenderFunc&& func)
	{
		m_RenderQueue.push_back(func);
	}

	inline void Renderer::SubmitMesh()
	{
		//later
		Submit([]() {
		});
	}

	void Renderer::Dispatch() {
		for (RenderFunc func : m_RenderQueue) {
			func();
		}
	}

	void Renderer::Destroy()
	{
		glfwTerminate();
	}

	RefLucy<RendererAPI> Renderer::GetRendererAPI()
	{
		return m_RendererAPI;
	}

	Scene& Renderer::GetActiveScene()
	{
		return m_Scene;
	}

	RenderContextType Renderer::GetCurrentRenderContextType()
	{
		return m_RenderContext->GetRenderContextType();
	}

	RefLucy<FrameBuffer>& Renderer::GetMainFrameBuffer()
	{
		return m_MainFrameBuffer;
	}

}