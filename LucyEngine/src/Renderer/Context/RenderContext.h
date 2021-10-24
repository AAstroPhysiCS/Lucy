#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	enum class RenderContextType {
		OpenGL,
		Vulkan
	};

	class RenderContext
	{
	public:
		virtual void Init() = 0;
		virtual void Destroy() = 0;
		virtual void PrintInfo() = 0;
		RenderContextType GetRenderContextType();

		static RefLucy<RenderContext> Create(RenderContextType type);

	protected:
		RenderContext(RenderContextType type);
		RenderContextType m_RenderContextType;
	};
}

