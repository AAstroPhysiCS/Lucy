#pragma once

#include "RenderPass.h"

namespace Lucy {

	class OpenGLRenderPass : public RenderPass {
	public:
		explicit OpenGLRenderPass(const RenderPassSpecification& specs);
		virtual ~OpenGLRenderPass() = default;

		void Begin(RenderPassBeginInfo& info) override;
		void End(RenderPassEndInfo& info) override;
	};
}
