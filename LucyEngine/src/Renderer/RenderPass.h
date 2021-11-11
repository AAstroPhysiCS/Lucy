#pragma once

#include "../Core/Base.h"
#include "Buffer/FrameBuffer.h"
#include "Context/Pipeline.h"

namespace Lucy {

	struct ClearColor {
		float r = 0, g = 0, b = 0, a = 0;
	};

	struct RenderPassSpecification {
		RefLucy<FrameBuffer> FrameBuffer;
		RefLucy<Pipeline> Pipeline;
		ClearColor ClearColor;
	};

	class RenderPass {
	public:
		static RefLucy<RenderPass> Create(RenderPassSpecification& specs);
		static void Begin(RefLucy<RenderPass>& renderPass);
		static void End(RefLucy<RenderPass>& renderPass);

		RenderPass(RenderPassSpecification& specs);

		inline RefLucy<FrameBuffer> GetFrameBuffer() { return m_Specs.FrameBuffer; }
		inline RefLucy<Pipeline> GetPipeline() { return m_Specs.Pipeline; }
		inline ClearColor GetClearColor() { return m_Specs.ClearColor; }

	private:
		RenderPassSpecification m_Specs;
	};
}

