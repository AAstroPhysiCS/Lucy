#pragma once

#include "../Core/Base.h"
#include "Context/Pipeline.h"

namespace Lucy {

	class RenderPass;

	class RenderCommand {
	public:
		static RefLucy<RenderCommand> Create();

		virtual void Begin(RefLucy<Pipeline> pipeline) = 0;
		virtual void End(RefLucy<Pipeline> pipeline) = 0;

		friend class Mesh;
	protected:
		RenderCommand() = default;

		static RefLucy<Pipeline> s_ActivePipeline;
	};

}

