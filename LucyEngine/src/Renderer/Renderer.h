#pragma once

#include "../Core/Base.h"
#include "../Scene/Scene.h"

#include "Context/RendererAPI.h"
#include "Context/RenderContext.h"

namespace Lucy {
	
	class FrameBuffer;
	class ShaderLibrary;

	class Renderer
	{
		using Func = std::function<void()>;

	public:
		static void Init(RenderContextType rendererContext);
		static void Destroy();

		inline static Scene& GetActiveScene() { return m_Scene; }
		inline static RenderContextType GetCurrentRenderContextType() { return s_RenderContext->GetRenderContextType(); }
		inline static RefLucy<FrameBuffer>& GetMainFrameBuffer() { return s_MainFrameBuffer; }
		inline static ShaderLibrary& GetShaderLibrary(){ return m_ShaderLibrary; }

		static void Submit(const Func&& func);
		static void SubmitMesh();

		static void Dispatch();
	private:
		static RefLucy<RendererAPI> s_RendererAPI;
		static RefLucy<RenderContext> s_RenderContext;
		static RefLucy<FrameBuffer> s_MainFrameBuffer;
		
		static Scene m_Scene;
		static std::vector<Func> s_RenderQueue;

		static ShaderLibrary m_ShaderLibrary;

		friend class RenderCommand;

		Renderer() = delete;
		~Renderer() = delete;
	};

}