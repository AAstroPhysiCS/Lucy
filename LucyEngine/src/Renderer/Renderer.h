#pragma once

#include "../Core/Base.h"
#include "../Scene/Scene.h"

#include "Context/RendererAPI.h"
#include "Context/RenderContext.h"

namespace Lucy {
	
	class FrameBuffer;

	class Renderer
	{
		using Func = std::function<void()>;

	public:
		static void Init(RenderContextType rendererContext);
		static void Destroy();

		inline static Scene& GetActiveScene() { return m_Scene; }
		inline static RenderContextType GetCurrentRenderContextType() { return m_RenderContext->GetRenderContextType(); }

		inline static RefLucy<FrameBuffer>& GetMainFrameBuffer() { return m_MainFrameBuffer; }

		static void Submit(const Func&& func);
		static void SubmitMesh();

		static void Dispatch();
	private:
		static RefLucy<RendererAPI> m_RendererAPI;
		static RefLucy<RenderContext> m_RenderContext;
		static RefLucy<FrameBuffer> m_MainFrameBuffer;
		
		static Scene m_Scene;
		static std::vector<Func> m_RenderQueue;

		friend class RenderCommand;

		Renderer() = delete;
		~Renderer() = delete;
	};

}