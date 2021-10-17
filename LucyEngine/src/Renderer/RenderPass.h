#pragma once

#include "../Core/Base.h"
#include "Pipeline.h"
#include "Buffer/FrameBuffer.h"

namespace Lucy {

	struct RenderPassSpecification {
		RefLucy<FrameBuffer> frameBuffer;
		Pipeline* pipeline = nullptr;

		struct ClearColor {
			float r = 0, g = 0, b = 0, a = 0;
		} clearColor;
	};

	class RenderPass
	{
	public:
		static RenderPass Create(RenderPassSpecification& specs);
		static void Begin(RefLucy<RenderPass>& renderPass);
		static void End(RefLucy<RenderPass>& renderPass);

		RenderPass(RenderPassSpecification& specs);
	private:
		RenderPassSpecification m_Specs;
	};
}

