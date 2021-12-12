#pragma once

#include "RenderCommand.h"

namespace Lucy {

	class OpenGLRenderCommand : public RenderCommand {
	public:
		OpenGLRenderCommand() = default;
		virtual ~OpenGLRenderCommand() = default;
		
		void Begin(RefLucy<RenderPass> renderPass);
		void End(RefLucy<RenderPass> renderPass);

		void ClearColor(float r, float g, float b, float a);
		void Clear(uint32_t bitField);

		void DrawElements(uint32_t count, uint32_t indices);
		void DrawElementsBaseVertex(uint32_t count, uint32_t indices, int32_t basevertex);

		void ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput);
		void ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode);
		void ReadBuffer(uint32_t mode);
	};
}

