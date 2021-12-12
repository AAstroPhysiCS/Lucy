#pragma once

#include "Context/RendererAPI.h"

namespace Lucy {

	class VulkanRenderer : public RendererAPI {
	public:
		VulkanRenderer(RenderArchitecture renderArchitecture);
		virtual ~VulkanRenderer() = default;

		void Init() override;

		void ClearCommands();
		void Draw();
		void Destroy();
		void Dispatch();

		void BeginScene(Scene & scene);
		void EndScene();

		void Submit(const Func && func);
		void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4 & entityTransform);

		void OnFramebufferResize(float sizeX, float sizeY);
		Entity OnMousePicking();
	private:

		friend class Renderer;
	};
}

