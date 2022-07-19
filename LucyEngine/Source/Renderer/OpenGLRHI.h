#pragma once

#include "Context/RHI.h"

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
		void EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform) override;

		void RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<Ref<DrawCommand>>&& func) override;

		void BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) override;

		inline Ref<OpenGLPipeline>& GetGeometryPipeline() { return m_GeometryPipeline; }
		inline Ref<OpenGLPipeline>& GetIDPipeline() { return m_IDPipeline; }
		
		void OnWindowResize() override;
		void OnViewportResize() override;
		Entity OnMousePicking() override;
	private:
		void GeometryPass();
		void IDPass();

		Ref<OpenGLPipeline> m_GeometryPipeline = nullptr;
		Ref<OpenGLPipeline> m_IDPipeline = nullptr;

		friend class Renderer;
	};

	class OpenGLAPICommands {
	public:
		static void ClearColor(float r, float g, float b, float a);
		static void Clear(uint32_t bitField);

		static void DrawElements(Topology topology, uint32_t count, uint32_t indices);
		static void DrawElementsBaseVertex(Topology topology, uint32_t count, uint32_t indices, int32_t basevertex);

		static void ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* meshIDOutput);
		static void ReadBuffer(Ref<OpenGLFrameBuffer> frameBuffer, uint32_t mode);
		static void ReadBuffer(uint32_t mode);
	};
}