#pragma once

#include "RenderPass.h"

namespace Lucy {

	class OpenGLRenderPass : public RenderPass {
	public:
		OpenGLRenderPass(RenderPassSpecification& specs);
		virtual ~OpenGLRenderPass() = default;

		void Begin(RenderPassBeginInfo& info);
		void End(RenderPassEndInfo& info);
	};
}
