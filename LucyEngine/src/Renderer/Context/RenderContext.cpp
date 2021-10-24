#include "RenderContext.h"
#include "../../Core/Base.h"
#include "OpenGLContext.h"

namespace Lucy {

	RenderContext::RenderContext(RenderContextType type)
	{
		m_RenderContextType = type;
	}

	RenderContextType RenderContext::GetRenderContextType()
	{
		return m_RenderContextType;
	}

	RefLucy<RenderContext> RenderContext::Create(RenderContextType type)
	{
		switch (type) {
			case RenderContextType::OpenGL:
				return CreateRef<OpenGLContext>(type);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
	}
}
