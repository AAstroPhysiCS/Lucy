#pragma once

#include "glad/glad.h"
#include "Context/RendererAPI.h"

#include "../Core/Base.h"

#include "../Scene/Scene.h"

namespace Lucy {

	enum class RendererContext {
		OPENGL,
		VULKAN
	};

	class Renderer
	{
	public:
		static void Init(RendererContext rendererContext);
		static void Destroy();

		static RendererContext GetCurrentContext();
		static RefLucy<RendererAPI> GetRendererAPI();

		static Scene& GetActiveScene();

	private:
		
		static void PrintInfo();
		
		static RendererContext m_RendererContext;
		static RefLucy<RendererAPI> m_RendererAPI;

		static Scene m_Scene;

		Renderer() = delete;
		~Renderer() = delete;
	};

}