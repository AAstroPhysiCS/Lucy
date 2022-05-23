#pragma once

#include "Context/RHI.h"
#include "../Core/Window.h"

#include "Context/OpenGLPipeline.h"

namespace Lucy {

	class OpenGLRHI : public RHI {
	public:
		OpenGLRHI(RenderArchitecture renderArchitecture);
		virtual ~OpenGLRHI() = default;

		void Init() override;
		void Destroy() override;
		void Dispatch() override;

		void BeginScene(Scene& scene) override;
		void RenderScene() override;
		PresentResult EndScene() override;

		void Enqueue(const SubmitFunc&& func) override;
		void EnqueueStaticMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) override;

		void RecordToCommandQueue(RecordFunc<MeshDrawCommand>&& func);

		void BindPipeline(RefLucy<Pipeline> pipeline);
		void UnbindPipeline(RefLucy<Pipeline> pipeline);
		void BindBuffers(RefLucy<VertexBuffer> vertexBuffer, RefLucy<IndexBuffer> indexBuffer);

		inline RefLucy<OpenGLPipeline>& GetGeometryPipeline() { return m_GeometryPipeline; }
		inline RefLucy<OpenGLPipeline>& GetIDPipeline() { return m_IDPipeline; }
		
		void OnViewportResize() override;
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
		static void ReadBuffer(RefLucy<OpenGLFrameBuffer> frameBuffer, uint32_t mode);
		static void ReadBuffer(uint32_t mode);
	};
}