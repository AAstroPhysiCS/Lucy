#pragma once

#include "Core/Base.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"

#include "GLFW/glfw3.h"

#include "Core/Module.h"
#include "Core/Panel.h"

#include "Core/Window.h"
#include "Scene/Scene.h"

#include "Renderer/VulkanDescriptors.h"

namespace Lucy {

	class VulkanDescriptorPool;

	class ImGuiOverlay {
	public:
		ImGuiOverlay();
		~ImGuiOverlay() = default;

		void Init(Ref<Window> window, Scene& scene);
		void Render();
		void OnEvent(Event& e);
		void Destroy();
	private:
		void Begin();
		void SendImGuiDataToGPU();
		void End();

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
		Ref<VulkanDescriptorPool> m_ImGuiPool = nullptr;

		Scene* m_Scene = nullptr;
		uint32_t m_Time = 0;
		std::vector<Panel*> m_Panels;
	};
}