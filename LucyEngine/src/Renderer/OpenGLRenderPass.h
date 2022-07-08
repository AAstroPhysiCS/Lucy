#pragma once

#include "RenderPass.h"

namespace Lucy {

	class OpenGLFrameBuffer;

	class OpenGLRenderPass : public RenderPass {
	public:
		OpenGLRenderPass(const RenderPassCreateInfo& createInfo);
		virtual ~OpenGLRenderPass() = default;

		void Begin(RenderPassBeginInfo& info) override;
		void End() override;
		void Destroy() override;
		void Recreate() override;
	private:
		Ref<OpenGLFrameBuffer> m_BoundedFrameBuffer = nullptr;
	};
}
