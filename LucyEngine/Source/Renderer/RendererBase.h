#pragma once

#include "Context/RenderContext.h"
#include "Device/RenderDevice.h"

namespace Lucy {

	class Window;
	class VulkanPushConstant;
	class DescriptorSet;

	class Scene;
	class Entity;

	class RendererBase {
	protected:
		uint32_t m_MaxFramesInFlight = 0;
		uint32_t m_ImageIndex = 0;
		uint32_t m_CurrentFrameIndex = 0;
	public:
		static Ref<RendererBase> Create(RenderArchitecture arch, Ref<Window>& window);
		virtual ~RendererBase() = default;
		
		void Init();

		virtual void BeginScene(Ref<Scene>& scene) = 0;
		virtual void RenderScene() = 0;
		virtual RenderContextResultCodes EndScene() = 0;
		virtual void WaitForDevice() = 0;

		virtual void Destroy();

		inline const Ref<RenderDevice>& GetRenderDevice() const { return m_RenderDevice; }
		inline const Ref<RenderContext>& GetRenderContext() const { return m_RenderContext; }

		inline const uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }
		inline const uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		inline const uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }

		virtual void OnWindowResize() = 0;
		virtual void OnViewportResize() = 0;
		virtual Entity OnMousePicking(Ref<Scene>& scene, const Ref<GraphicsPipeline>& idPipeline) = 0;
	protected:
		RendererBase(RenderArchitecture arch, Ref<Window>& window);

		Ref<RenderContext> m_RenderContext = nullptr;
		Ref<RenderDevice> m_RenderDevice = nullptr;
	};
}