#pragma once

#include "Core/Base.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"

#include "GLFW/glfw3.h"

#include "Core/Layer.h"
#include "UI/Panel.h"

#include "Core/Window.h"

#include "Renderer/VulkanDescriptors.h"
#include "Renderer/Context/VulkanPipeline.h"

namespace Lucy {

	class VulkanRenderPass;
	class VulkanDescriptorPool;

	class ImGuiOverlay {
	public:
		static ImGuiOverlay& GetInstance() {
			static ImGuiOverlay s_Instance;
			return s_Instance;
		}

		void Init(RefLucy<Window> window);
		void Begin(PerformanceMetrics& performanceMetrics);
		void End();
		void OnRender();
		void OnEvent(Event& e);
		void Destroy();
	private:
		ImGuiOverlay();
		~ImGuiOverlay() = default;

		void UIPass();
		inline bool IsInitiated() { return ImGui::GetIO().BackendRendererUserData; }

		std::vector<VkDescriptorPoolSize> m_ImGuiPoolSizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VulkanDescriptorPoolSpecifications m_PoolSpecs = { m_ImGuiPoolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1000 };
		RefLucy<VulkanDescriptorPool> m_ImGuiPool = nullptr;

		uint32_t m_Time = 0;
		std::vector<Panel*> m_Panels;
	};
}