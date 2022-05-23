#pragma once

#include "RenderPass.h"

namespace Lucy {

	class OpenGLFrameBuffer;

	class OpenGLRenderPass : public RenderPass {
	public:
		OpenGLRenderPass(const RenderPassSpecification& specs);
		virtual ~OpenGLRenderPass() = default;

		void Begin(RenderPassBeginInfo& info) override;
		void End() override;
		void Destroy() override;
		void Recreate() override;

	private:
		RefLucy<OpenGLFrameBuffer> m_BoundedFrameBuffer = nullptr;
	};
}
