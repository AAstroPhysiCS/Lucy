#pragma once

#include "../Core/Base.h"
#include "GLFW/glfw3.h"

#include "Buffer/FrameBuffer.h"

namespace Lucy {

	class RenderPass;

	class RenderCommand {
	public:
		static RefLucy<RenderCommand> Create();

		virtual void Begin(RefLucy<RenderPass> renderPass) = 0;
		virtual void End(RefLucy<RenderPass> renderPass) = 0;

		friend class Mesh;
	protected:
		RenderCommand() = default;

		static RenderPass* s_ActiveRenderPass;
	};

}

