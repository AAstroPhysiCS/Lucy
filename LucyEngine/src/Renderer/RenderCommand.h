#pragma once

#include "../Core/Base.h"
#include "GLFW/glfw3.h"

namespace Lucy {

	class RenderPass;

	class RenderCommand {
	public:
		static void Begin(RefLucy<RenderPass> renderPass);
		static void End(RefLucy<RenderPass> renderPass);

		static void ClearColor(float r, float g, float b, float a);
		static void Clear(uint32_t bitField);

		static void DrawElements(uint32_t count, uint32_t indices);
		static void DrawElementsBaseVertex(uint32_t count, uint32_t indices, int32_t basevertex);

		static void ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput);
		static void ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode);
		static void ReadBuffer(uint32_t mode);
		static void SwapBuffers(GLFWwindow* window);

		friend class Mesh;
	private:
		RenderCommand() = default;

		static RenderPass* s_ActiveRenderPass;
	};

}

