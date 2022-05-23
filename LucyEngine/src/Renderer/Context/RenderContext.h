#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	enum class RenderArchitecture {
		OpenGL,
		Vulkan
	};

	class RenderContext {
	protected:
		RenderContext() = default;
		~RenderContext() = default;

		virtual void Init() = 0;
	public:
		virtual void Destroy() = 0;
		virtual void PrintInfo() = 0;

		static RefLucy<RenderContext> Create(RenderArchitecture arch);
	};
}

