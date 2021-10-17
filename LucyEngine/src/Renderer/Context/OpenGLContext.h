#pragma once

#include "RenderContext.h"

namespace Lucy {

	class OpenGLContext : public RenderContext
	{
	public:
		OpenGLContext(RenderContextType type);
		virtual ~OpenGLContext() = default;

		void Init();
		void Destroy();
		void PrintInfo();
	};
}

