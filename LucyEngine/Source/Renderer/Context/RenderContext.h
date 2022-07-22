#pragma once

#include "Core/Base.h"

#include "../RenderArchitecture.h"

namespace Lucy {

	class RenderContext {
	public:
		virtual ~RenderContext() = default;
		
		virtual void Destroy() = 0;
		virtual void PrintInfo() = 0;

		static Ref<RenderContext> Create(RenderArchitecture arch);
	protected:
		virtual void Init() = 0;

		RenderContext() = default;
	};
}

