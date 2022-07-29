#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"

#include "Core/Panel.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

#define IMGUI_MAX_POOL_SIZES 100

	class RendererModule;
	class VulkanDescriptorPool;

	class FrameBuffer;
	class RenderPass;
	
	class Scene;
	class Window;

	struct ImGuiPipeline {
		Ref<RenderPass> UIRenderPass = nullptr;
		Ref<FrameBuffer> UIFramebuffer = nullptr;
	};

	class ImGuiOverlay {
	public:
		ImGuiOverlay();
		~ImGuiOverlay() = default;

		void Init(Ref<Window> window, Ref<Scene> scene, const Ref<RendererModule>& rendererModule);
		void Render();
		void SendImGuiDataToDevice();
		void OnEvent(Event& e);
		void Destroy();
	private:
		void Begin();
		void End();

		VulkanDescriptorPoolCreateInfo m_PoolSpecs = { {}, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, IMGUI_MAX_POOL_SIZES};
		Ref<VulkanDescriptorPool> m_ImGuiPool = nullptr;

		uint32_t m_Time = 0;
		std::vector<Panel*> m_Panels;

		ImGuiPipeline m_ImGuiPipeline;
	};
}