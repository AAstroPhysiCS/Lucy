#pragma once

#include "Core/Base.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"

#include "GLFW/glfw3.h"

#include "Core/Panel.h"
#include "Core/Window.h"

#include "Scene/Scene.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

#define IMGUI_MAX_POOL_SIZES 100

	class VulkanDescriptorPool;

	class ImGuiOverlay {
	public:
		ImGuiOverlay();
		~ImGuiOverlay() = default;

		void Init(Ref<Window> window, Ref<Scene> scene);
		void Render();
		void SendImGuiDataToGPU();
		void OnEvent(Event& e);
		void Destroy();
	private:
		void Begin();
		void End();

		VulkanDescriptorPoolCreateInfo m_PoolSpecs = { {}, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, IMGUI_MAX_POOL_SIZES};
		Ref<VulkanDescriptorPool> m_ImGuiPool = nullptr;

		uint32_t m_Time = 0;
		std::vector<Panel*> m_Panels;
	};
}