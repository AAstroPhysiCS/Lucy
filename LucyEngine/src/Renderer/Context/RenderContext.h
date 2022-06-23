#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	enum class RenderArchitecture {
		OpenGL,
		Vulkan
	};

	class RenderContext {
	public:
		virtual ~RenderContext() = default;
		
		virtual void Destroy() = 0;
		virtual void PrintInfo() = 0;

		static Ref<RenderContext> Create(RenderArchitecture arch);
	protected:
		RenderContext() = default;

		virtual void Init() = 0;
	};
}

