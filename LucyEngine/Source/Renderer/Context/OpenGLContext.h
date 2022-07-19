#pragma once

#include "RenderContext.h"

namespace Lucy {

	class OpenGLContext : public RenderContext {
	public:
		OpenGLContext();
		virtual ~OpenGLContext() = default;

		void Destroy() override;
		void PrintInfo() override;
	private:
		void Init() override;
	};
}

