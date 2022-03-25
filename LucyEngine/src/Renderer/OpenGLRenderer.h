#pragma once

#include "Context/RendererAPI.h"
#include "../Core/Window.h"

#include "Context/OpenGLPipeline.h"

namespace Lucy {

	class OpenGLRenderer : public RendererAPI {
	public:
		OpenGLRenderer(RenderArchitecture renderArchitecture);
		virtual ~OpenGLRenderer() = default;

		void Init() override;

		void ClearCommands() override;
		void Execute() override;
		void Destroy() override;
		void Dispatch() override;

		void BeginScene(Scene& scene) override;
		void EndScene() override;

		void Submit(const Func&& func) override;
		void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) override;

		inline RefLucy<OpenGLPipeline>& GetGeometryPipeline() { return m_GeometryPipeline; }
		inline RefLucy<OpenGLPipeline>& GetIDPipeline() { return m_IDPipeline; }
		
		void OnFramebufferResize(float sizeX, float sizeY) override;
		Entity OnMousePicking() override;
	private:
		void GeometryPass();
		void IDPass();

		RefLucy<OpenGLPipeline> m_GeometryPipeline;
		RefLucy<OpenGLPipeline> m_IDPipeline;

		friend class Renderer;
	};

	class OpenGLAPICommands {
	public:
		static void ClearColor(float r, float g, float b, float a);
		static void Clear(uint32_t bitField);

		static void DrawElements(Topology topology, uint32_t count, uint32_t indices);
		static void DrawElementsBaseVertex(Topology topology, uint32_t count, uint32_t indices, int32_t basevertex);

		static void ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput);
		static void ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode);
		static void ReadBuffer(uint32_t mode);
	};
}