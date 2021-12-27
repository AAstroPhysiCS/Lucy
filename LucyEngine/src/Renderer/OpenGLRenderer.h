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

		void ClearCommands();
		void Draw();
		void Destroy();
		void Dispatch();

		void BeginScene(Scene& scene);
		void EndScene();

		void Submit(const Func&& func);
		void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform);

		inline RefLucy<OpenGLPipeline>& GetGeometryPipeline() { return m_GeometryPipeline; }
		inline RefLucy<OpenGLPipeline>& GetIDPipeline() { return m_IDPipeline; }
		
		void OnFramebufferResize(float sizeX, float sizeY);
		Entity OnMousePicking();
	private:
		void GeometryPass();
		void IDPass();

		RefLucy<OpenGLPipeline> m_GeometryPipeline;
		RefLucy<OpenGLPipeline> m_IDPipeline;

		friend class Renderer;
	};
}