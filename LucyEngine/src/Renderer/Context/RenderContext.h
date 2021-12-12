#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	enum class RenderArchitecture {
		OpenGL,
		Vulkan
	};

	class RenderContext {
	public:
		virtual void Destroy() = 0;
		virtual void PrintInfo() = 0;

		static RefLucy<RenderContext> Create(RenderArchitecture type);
	protected:
		RenderContext(RenderArchitecture type);

		virtual void Init(RenderArchitecture type) = 0;
	};
}

