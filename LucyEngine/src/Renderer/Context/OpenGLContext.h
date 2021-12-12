#pragma once

#include "RenderContext.h"

namespace Lucy {

	class OpenGLContext : public RenderContext {
	public:
		OpenGLContext(RenderArchitecture type);
		virtual ~OpenGLContext() = default;

		void Destroy();
		void PrintInfo();
	private:
		void Init(RenderArchitecture type);
	};
}

