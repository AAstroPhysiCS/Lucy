#pragma once

#include "RenderContext.h"

#include "vulkan/vulkan.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Context/VulkanSwapChain.h"

namespace Lucy {

	struct VulkanMessageCallback {
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
															VkDebugUtilsMessageTypeFlagsEXT messageType,
															const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
															void* pUserData);
		static void ImGui_DebugCallback(VkResult result);
	};

	class VulkanContext : public RenderContext {
	public:
		VulkanContext(RenderArchitecture type);
		virtual ~VulkanContext() = default;

		void Destroy() override;
		void PrintInfo() override;
		inline VkInstance GetVulkanInstance() { return m_Instance; };
	private:
		void Init(RenderArchitecture type) override;
		void CheckValidationSupport();
		void SetupMessageCallback();
		void DestroyMessageCallback();

		std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger{};
		
		friend class VulkanRenderer;
		friend class Renderer;
	};
}