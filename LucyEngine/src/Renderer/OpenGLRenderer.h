#pragma once

#include "Context/RendererAPI.h"
#include "../Core/Window.h"

#include "OpenGLRenderPass.h"

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

		inline RefLucy<OpenGLRenderPass>& GetGeometryPass() { return m_GeometryPass; }
		inline RefLucy<OpenGLRenderPass>& GetIDPass() { return m_IDPass; }
		
		void OnFramebufferResize(float sizeX, float sizeY);
		Entity OnMousePicking();
	private:
		void GeometryPass();
		void IDPass();

		RefLucy<OpenGLRenderPass> m_GeometryPass;
		RefLucy<OpenGLRenderPass> m_IDPass;

		friend class Renderer;
	};
}