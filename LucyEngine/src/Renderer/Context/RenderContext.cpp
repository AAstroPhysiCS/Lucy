#include "RenderContext.h"
#include "../../Core/Base.h"
#include "OpenGLContext.h"

namespace Lucy {

	RenderContext::RenderContext(RenderAPI type)
		: m_RenderContextType(type) {
	}

	RenderAPI RenderContext::GetRenderAPI() {
		return m_RenderContextType;
	}

	RefLucy<RenderContext> RenderContext::Create(RenderAPI type) {
		switch (type) {
			case RenderAPI::OpenGL:
				return CreateRef<OpenGLContext>(type);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
	}
}
