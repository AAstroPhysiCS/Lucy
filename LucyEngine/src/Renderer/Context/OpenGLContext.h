#pragma once

#include "RenderContext.h"

namespace Lucy {

	class OpenGLContext : public RenderContext {
	public:
		explicit OpenGLContext(RenderArchitecture type);
		virtual ~OpenGLContext() = default;

		void Destroy() override;
		void PrintInfo() override;
	private:
		void Init(RenderArchitecture type) override;
	};
}

