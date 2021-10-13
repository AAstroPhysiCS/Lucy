#pragma once

#include "glad/glad.h"

#include "../Core/Base.h"
#include "../Scene/Scene.h"

#include "Context/RendererAPI.h"
#include "Context/RenderContext.h"

namespace Lucy {

	class FrameBuffer;
	using RenderFunc = std::function<void()>;

	class Renderer
	{
	public:
		static void Init(RenderContextType rendererContext);
		static void Destroy();

		static RefLucy<RendererAPI> GetRendererAPI();
		static Scene& GetActiveScene();
		static RenderContextType GetCurrentRenderContextType();
		
		static RefLucy<FrameBuffer>& GetMainFrameBuffer();

		static inline void Submit(const RenderFunc&& func);
		static inline void SubmitMesh();

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