#include "RendererAPI.h"

#include "../Renderer.h"
#include "OpenGLRendererAPI.h"

namespace Lucy {

	RefLucy<RendererAPI> RendererAPI::Create() {
		switch (Renderer::GetCurrentRenderAPI()) {
			case RenderAPI::OpenGL:
				return CreateRef<OpenGLRendererAPI>();
			default:
				LUCY_CRITICAL("API not supported!");
				LUCY_ASSERT(false);
		}
	}
}