#pragma once

#include "glad/glad.h"

#include "../Core/Base.h"
#include "../Scene/Scene.h"

#include "Context/RendererAPI.h"
#include "Context/RenderContext.h"

namespace Lucy {

	using RenderFunc = std::function<void()>;
	
	class FrameBuffer;

	class Renderer
	{
	public:
		static void Init(RenderContextType rendererContext);
		static void Destroy();

		inline static RefLucy<RendererAPI>& GetRendererAPI() { return m_RendererAPI; }
		inline static Scene& GetActiveScene() { return m_Scene; }
		inline static RenderContextType GetCurrentRenderContextType() { return m_RenderContext->GetRenderContextType(); }
		
		inline static RefLucy<FrameBuffer>& GetMainFrameBuffer() { return m_MainFrameBuffer; }

		static void Submit(const RenderFunc&& func);
		static void SubmitMesh();

		static void Dispatch();
	private:
		static RefLucy<RendererAPI> m_RendererAPI;
		static RefLucy<RenderContext> m_RenderContext;
		static RefLucy<FrameBuffer> m_MainFrameBuffer;
		
		static Scene m_Scene;
		static std::vector<RenderFunc> m_RenderQueue;

		Renderer() = delete;
		~Renderer() = delete;
	};

}