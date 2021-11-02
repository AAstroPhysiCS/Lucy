#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	enum class RenderAPI {
		OpenGL,
		Vulkan
	};

	class RenderContext
	{
	public:
		virtual void Init() = 0;
		virtual void Destroy() = 0;
		virtual void PrintInfo() = 0;
		RenderAPI GetRenderAPI();

		static RefLucy<RenderContext> Create(RenderAPI type);

	protected:
		RenderContext(RenderAPI type);
		RenderAPI m_RenderContextType;
	};
}

