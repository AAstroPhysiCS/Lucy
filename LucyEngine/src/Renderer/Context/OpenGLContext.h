#pragma once

#include "RenderContext.h"

namespace Lucy {

	class OpenGLContext : public RenderContext {
	public:
		OpenGLContext(RenderAPI type);
		virtual ~OpenGLContext() = default;

		void Init();
		void Destroy();
		void PrintInfo();
	};
}

