#pragma once

#include "glad/glad.h"
#include "RendererAPI.h"

#include "../Core/Base.h"
#include "OpenGL/OpenGLRendererAPI.h"

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

	private:
		
		static void PrintInfo();
		
		static RendererContext m_RendererContext;
		static RefLucy<RendererAPI> m_RendererAPI;

		Renderer() = delete;
		~Renderer() = delete;
	};

}